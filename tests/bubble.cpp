#include "common.hpp"
#include "require.hpp"
#include "MPSCQueue.hpp"
#include "NonMovableArray.hpp"
#include "QuiescenceBarrier.hpp"
#include "Service.hpp"

#include "tree/tree.hpp"
#include "ingest/mmio.hpp"

#include <CLI/App.hpp>

#include <algorithm>
#include <barrier>
#include <bit>
#include <chrono>
#include <deque>
#include <format>
#include <print>
#include <thread>
#include <utility>
#include <vector>

using namespace tree;
using namespace tree::tests;

static constexpr auto service_to_consumer(u32 i, u32 n_services, u32 n_consumers) -> u32 {
	auto const d = n_services / n_consumers + !!(n_services % n_consumers);
	return i / d;
}	

namespace
{
	struct ConsumerQueues : NonMovableArray<MPSCQueue>
	{
		constexpr ConsumerQueues(u32 n_consumers, u32 n_producers, u32 size)
				: NonMovableArray(n_consumers, n_producers + 1, size)
		{
		}

		constexpr auto get_rx_endpoint(this ConsumerQueues& self, std::size_t i) -> MPSCQueue::RxEndpoint {
			return self[i].get_rx_endpoint();
		}
		
		constexpr auto get_tx_endpoints(this ConsumerQueues& self) -> std::vector<MPSCQueue::TxEndpoint> {
			std::vector<MPSCQueue::TxEndpoint> out;
			out.reserve(self.size());
			for (auto&& q : self) {
				out.emplace_back(q.get_tx_endpoint());
			}
			return out;
		}
	};

	struct Services : NonMovableArray<Service>
	{
		using NonMovableArray::NonMovableArray;
	};
}

auto main(int argc, char** argv) -> int
{
	CLI::App app;
	options::process_command_line(app);

	u32 n_consumers = 1_u32;
	u32 n_producers = 1_u32;
	u32 n_services = 1_u32;
	u64 n_edges = -1_u64;
	u32 queue_size = 512_u32;
	bool validate = true;
	std::string path{};

	app.add_option("path", path, "The path to the mmio file")->required();
	app.add_option("n_edges", n_edges, "The number of edges to process (default: all)");
	app.add_option("-c, --n_consumers", n_consumers, std::format("The number of threads to use as consumers (default: {})", n_consumers));
	app.add_option("-p, --n_producers", n_producers, std::format("The number of threads to use as producers (default: {})", n_producers));
	app.add_option("-q, --queue_size", queue_size, std::format("The consumer queue size (default: {})", queue_size));
	app.add_option("-n, --n_services", n_services, "The number of services to provision (default: 1)");
	app.add_flag("--validate,!--no-validate", validate, "Run the validation code (default: true)");
	
	CLI11_PARSE(app, argc, app.ensure_utf8(argv));
	
	require(1 <= n_consumers);
	require(1 <= n_producers);
	require(1 <= n_services);

	auto const n_edges_per_producer = n_edges / n_producers;

	options::print_options(stdout);

	std::print("\n");
	std::print("         n_consumers: {}\n", n_consumers);
	std::print("         n_producers: {}\n", n_producers);
	std::print("          n_services: {}\n", n_services);
	std::print("             n_edges: {}\n", n_edges);
	std::print("n_edges_per_producer: {}\n", n_edges_per_producer);
	std::print("          queue_size: {}\n", queue_size);
	std::print("            validate: {}\n", validate);
	std::print("\n");
	std::fflush(stdout);
	
	auto tlt = TopLevelTree(n_services);
	auto services = Services(n_services, tlt);
	auto queues = ConsumerQueues(n_consumers, n_producers, queue_size);
	auto bubbles = MPSCBlockingQueue(); //MPSCBlockingQueue(n_consumers, 512);
	
	auto done_producing = std::atomic_flag(false);
	auto cleanup = QuiescenceBarrier(n_consumers + 1);

	// Allocate barriers to be used by the consumer and producer.
	auto consumer_barrier = std::barrier(n_consumers + 1);
	auto producer_barrier = std::barrier(n_producers + 1);

	// This will store all the threads we spawn below.
	auto threads = std::vector<std::jthread>();

	// Start the consumer threads.
	for (u32 i = 0; i < n_consumers; ++i) {
		threads.emplace_back([&,id=i]
		{
			std::print("starting consumer {}\n", id);

			auto rx = queues.get_rx_endpoint(id);
			auto tx = bubbles.get_tx_endpoint();
			
			consumer_barrier.arrive_and_wait();
			
			u64 n = 0;
			auto handle_requests = [&]
			{
				int active = 0;
				while (auto key = rx.try_dequeue()) {
					auto const service = tlt.lookup(*key);

					// this key might have been moved
					if (id != service_to_consumer(service, n_services, n_consumers)) {
						std::print("discovered old key {:032x} {}\n", *key, id);
						tx.enqueue(*key);
						continue;
					}

					// try to insert this 
					if (auto result = services[service].insert(*key); not result) {
						tx.enqueue(result.error());
						continue;
					}
					
					n += 1;
					active += 1;
				}
				return active;
			};
			
			while (not done_producing.test()) {
				handle_requests();
			}

			cleanup("consumer", handle_requests);
			
			require(rx.size() == 0);
			
			consumer_barrier.arrive_and_wait();
						
			std::print("consumer {} processed {} keys ({} stalls)\n", id, n, rx.stalls);
		});
	}
	
	// Start the producer threads.
	for (u32 i = 0; i < n_producers; ++i) {
		threads.emplace_back([&,i=i]
		{
			std::print("starting producer {}\n", i);
			
			// Allocate a token for each of the queues.
			auto tx = queues.get_tx_endpoints();

			// Open the file for reading.
			auto mm = ingest::mmio::Reader(path, n_producers, i);			

			producer_barrier.arrive_and_wait();
			
			// Count the number of tuples that I'm processing, and the amount of
			// stalling I have to do.
			u64 n = 0;

			// Process each tuple.
			while (auto tuple = mm.next()) {
				if (n == n_edges_per_producer) break;
				
				auto const key = tuple_to_key(*tuple);
				auto const service = tlt.lookup(key);
				auto const consumer = service_to_consumer(service, n_services, n_consumers);
				require(consumer < n_consumers);
				tx[consumer].enqueue(key);
				n += 1;
			}

			producer_barrier.arrive_and_wait();

			u64 stalls{};
			for (auto& q : tx) {
				stalls += q.stalls;
			}
			
			// Dump output for this thread.
			std::print("producer {} processed {} keys ({} stalls)\n", i, n, stalls);
		});
	}

	threads.emplace_back([&]
	{
		auto out = std::fopen("bubble.out", "w");
		auto rx = bubbles.get_rx_endpoint();
		auto tx = queues.get_tx_endpoints();

		u64 n = 0;
		auto handle_requests = [&]
		{
			int active = 0;
			while (auto key = rx.try_dequeue()) {
				auto const service = tlt.lookup(*key);
				auto const consumer = service_to_consumer(service, n_services, n_consumers);
				tx[consumer].enqueue(*key);
				n += 1;
				active += 1;
			}
			return active;
		};

		while (not done_producing.test()) {
			handle_requests();
		}

		cleanup("bubble", handle_requests);

		std::fclose(out);
		
		std::print("bubble service transferred {} keys\n", n);
	});
	
	auto const start_time = std::chrono::steady_clock::now();
	consumer_barrier.arrive_and_wait();
	producer_barrier.arrive_and_wait();

	producer_barrier.arrive_and_wait();
	done_producing.test_and_set();
	consumer_barrier.arrive_and_wait();
	auto const end_time = std::chrono::steady_clock::now();

	// Join all of our threads.
	for (auto& thread : threads) {
		thread.join();
	}

	std::print("time: {}\n", end_time - start_time);
	
	if (validate) {
		u64 m = 0;
		for (u32 i = 0; i < n_producers; ++i) {
			auto mm = ingest::mmio::Reader(path, n_producers, i);
			u64 n = 0;
			while (auto tuple = mm.next()) {
				if (n == n_edges_per_producer) break;

				auto const key = tuple_to_key(*tuple);
				auto const service = tlt.lookup(key);
				if (not services[service].contains(key)) {
					std::print("failed to find {:032x} in {}\n", key, service);
					std::fflush(stdout);
					assert(false);
				}
				require(services[service].contains(key));

				n += 1;
				m += 1;
			}
		}
		std::print("validated {} tuples\n", m);
	}
}
