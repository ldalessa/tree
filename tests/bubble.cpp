#include "common.hpp"
#include "require.hpp"
#include "MPSCQueue.hpp"
#include "TopLevelTree.hpp"

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
	struct ConsumerQueues
	{
		std::deque<MPSCQueue> _queues;
		
		constexpr ConsumerQueues(u32 n_consumers, u32 n_producers, u32 size)
		{
			for (u32 i = 0; i < n_consumers; ++i) {
				_queues.emplace_back(n_producers + 1, size);
			}
		}

		constexpr auto get_rx_endpoint(std::size_t i) -> MPSCQueue::RxEndpoint {
			return _queues[i].get_rx_endpoint();
		}
		
		constexpr auto get_tx_endpoints() -> std::vector<MPSCQueue::TxEndpoint> {
			std::vector<MPSCQueue::TxEndpoint> out;
			out.reserve(_queues.size());
			for (auto&& q : _queues) {
				out.emplace_back(q.get_tx_endpoint());
			}
			return out;
		}
	};

	struct Service
	{
		TopLevelTree& _tlt;
		TopLevelTree _bubbled; // TODO: don't need concurrent tree
		GlobTreeNode _globs{"0/0"};

		constexpr Service(u32 n_services, TopLevelTree& tlt)
			: _tlt(tlt)
			, _bubbled(n_services, false)
		{
		}

		constexpr auto insert(u128 key) -> std::expected<bool, std::vector<u128>>
		{
			if (_bubbled.contains(key)) {
				return std::unexpected<std::vector<u128>>(std::in_place, 1, key);
			}
			
			return _globs.insert(key).transform_error([&](GlobTreeNode glob) -> std::vector<u128> {
				_bubbled.insert(glob.key());
				_tlt.insert(glob.key());
				return glob.take_value().take_all();
			});
		}

		constexpr auto contains(u128 key) -> bool {
			return _globs.find(key, nullptr);
		}
	};

	struct Services
	{
		using Allocator = std::allocator<Service>;
		using Traits = std::allocator_traits<Allocator>;
		
		Allocator _allocator{};
		Service* _services;
		u32 _n_services;

		constexpr ~Services() {
			for (u32 i = 0, j = _n_services - 1; i < _n_services; ++i, --j) {
				Traits::destroy(_allocator, _services + j);
			}
			Traits::deallocate(_allocator, _services, _n_services);
		}
		
		constexpr Services(u32 n_services, TopLevelTree& tlt)
			: _services(Traits::allocate(_allocator, n_services))
			, _n_services(n_services)
		{
			for (u32 i = 0; i < n_services; ++i) {
				Traits::construct(_allocator, _services + i, _n_services, tlt);
			}
		}

		constexpr Services(Service const&) = delete;
		constexpr Services(Service&&) = delete;
		
		constexpr auto operator[](std::size_t i) -> Service& {
			require(i < _n_services);
			return _services[i];
		}
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

	auto done_producing = std::atomic_flag(false);
	auto cleanup = std::barrier(n_consumers + 1);
	auto threads = std::vector<std::jthread>();

	// Allocate barriers to be used by the consumer and producer.
	auto consumer_barrier = std::barrier(n_consumers + 1);
	auto producer_barrier = std::barrier(n_producers + 1);
	
	// Start the consumer threads.
	for (u32 i = 0; i < n_consumers; ++i) {
		threads.emplace_back([&,id=i]
		{
			std::print("starting consumer {}\n", id);

			auto rx = queues.get_rx_endpoint(id);

			consumer_barrier.arrive_and_wait();
			
			u64 n = 0;
			auto handle_requests = [&] {
				while (auto key = rx.try_dequeue()) {
					auto const service = tlt.lookup(*key);
					require(service_to_consumer(service, n_services, n_consumers) == id);
					if (auto result = services[service].insert(*key); not result) {
						static int x = 0;
						std::print("dropping bubble {}\n", x++);
					}
					n += 1;
				}
			};
			
			while (not done_producing.test()) {
				handle_requests();
			}

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
				require(services[service].contains(key));

				n += 1;
				m += 1;
			}
		}
		std::print("validated {} tuples\n", m);
	}
}
