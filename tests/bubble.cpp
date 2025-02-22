#include "require.hpp"

#include "tree/tree.hpp"
#include "ingest/mmio.hpp"

#include <CLI/App.hpp>
#include <concurrentqueue.h>
#include <blockingconcurrentqueue.h>

#include <algorithm>
#include <barrier>
#include <bit>
#include <chrono>
#include <format>
#include <print>
#include <thread>
#include <utility>
#include <vector>

using namespace tree;
namespace mc = moodycamel;

static constexpr auto service_to_consumer(u32 i, u32 n_services, u32 n_consumers) -> u32 {
	auto const d = n_services / n_consumers + !!(n_services % n_consumers);
	return i / d;
}
	
static constexpr auto tuple_to_key(ingest::Tuple const& tuple) -> Key {
	return Key(tuple.k, tuple.b);
}

static constexpr auto close_mapping(Key const& key) -> u32 {
	return 0;
}

namespace
{
	struct BubbleRequest {
		Key key;
		Glob glob;
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

	
	auto tlt = TreeNode<u32>("0/0");
	// auto tlt = TopLevelTreeNode("0/0");
	// auto queues = std::vector<mc::BlockingConcurrentQueue<Key>>();
	auto bubbled = std::vector<TopLevelTreeNode>();
	auto queues = std::vector<mc::ConcurrentQueue<Key>>();
	auto done_producing = std::atomic_flag(false);
	auto cleanup = std::barrier(n_consumers + 1);
	auto threads = std::vector<std::jthread>();
	auto services = std::vector<GlobTreeNode>();

	auto bubble_service = mc::BlockingConcurrentQueue<BubbleRequest>();
	auto bubbles_created = std::atomic<u32>();
	auto bubbles_processed = std::atomic<u32>();
	
	// Allocate the queues.
	queues.reserve(n_consumers);
	for (u32 i = 0; i < n_consumers; ++i) {
		queues.emplace_back(queue_size, 0, n_producers + 1);//n_producers + 1, 0);
	}
	
	// Allocate local trees and the bubbles for each service.
	services.reserve(n_services);
	for (u32 i = 0; i < n_services; ++i) {
		services.emplace_back("0/0");
		bubbled.emplace_back("0/0");
	}

	// Allocate top level tree nodes for each service.
	u32 const n_bits = std::countr_zero(std::bit_ceil(n_services));
	for (u32 i = 0; i < n_services; ++i) {
		auto const j = u128(i);
		auto const key = Key(std::rotr(j, n_bits), n_bits);
		if (options::verbose) {
			std::print("inserting tlt entry {}->{}\n", key, i);
		}
		tlt.insert_or_update(key, i);
		require(tlt.find(key)->value() == i);
	}
	
	auto consumer_barrier = std::barrier(n_consumers + 1);

	std::mutex token_mutex;
	
	// Start the consumer threads.
	for (u32 i = 0; i < n_consumers; ++i) {
		threads.emplace_back([&,i=i]
		{
			std::print("starting consumer {}\n", i);

			auto token = [&] {
				auto const _ = std::unique_lock(token_mutex);
				return mc::ConsumerToken(queues[i]);
			}();

			auto bubble_token = [&] {
				auto const _ = std::unique_lock(token_mutex);
				return mc::ProducerToken(bubble_service);
			}();

			consumer_barrier.arrive_and_wait();
			
			u64 n = 0;
			u64 stalls = 0;

			auto handle_requests = [&] {
				Key key;
				while (queues[i].try_dequeue(token, key)) {
					auto const service = tlt.find(key)->value();
					if (bubbled[service].find(key, nullptr)) {
						// just drop it for now
						require(false);
						continue;
					}

					require(service_to_consumer(service, n_services, n_consumers) == i);
					try {
						require(services[service].insert(key));
						n += 1;
					} catch (GlobTreeNode& node) {
						bubbles_created += 1;
						bubbled[service].insert_or_update(node._key, 0);
						auto request = BubbleRequest(node.key(), node.take_value());
						while (not bubble_service.enqueue(bubble_token, std::move(request))) {
						}
					}
				}
			};
			
			while (not done_producing.test()) {
				handle_requests();
				stalls += 1;
			}

			do {
				cleanup.arrive_and_wait();
				handle_requests();
			} while (bubbles_created != bubbles_processed);

			require(queues[i].size_approx() == 0);
			
			consumer_barrier.arrive_and_wait();
						
			std::print("consumer {} processed {} keys ({} stalls)\n", i, n, stalls);
		});
	}

	auto producer_barrier = std::barrier(n_producers + 1);
	
	// Start the producer threads.
	for (u32 i = 0; i < n_producers; ++i) {
		threads.emplace_back([&,i=i]
		{
			std::print("starting producer {}\n", i);
			
			// Allocate a token for each of the queues.
			std::vector<mc::ProducerToken> tokens;
			tokens.reserve(queues.size());
			for (auto& q : queues) {
				auto const _ = std::unique_lock(token_mutex);
				tokens.emplace_back(q);
			}

			// Open the file for reading.
			auto mm = ingest::mmio::Reader(path, n_producers, i);			

			producer_barrier.arrive_and_wait();
			
			// Count the number of tuples that I'm processing, and the amount of
			// stalling I have to do.
			u64 n = 0;
			u64 stalls = 0;
			// Process each tuple.
			while (auto tuple = mm.next()) {
				if (n == n_edges_per_producer) break;
				
				auto const key = tuple_to_key(*tuple);
				auto const service = tlt.find(key)->value();
				auto const consumer = service_to_consumer(service, n_services, n_consumers);
				require(consumer < n_consumers);
				while (not queues[consumer].try_enqueue(tokens[consumer], key)) {
					stalls += 1;
				}
				n += 1;
			}

			producer_barrier.arrive_and_wait();
			
			// Dump output for this thread.
			std::print("producer {} processed {} keys ({} stalls)\n", i, n, stalls);
		});
	}

	// Bubble service.
	threads.emplace_back([&]
	{
		auto token = [&] {
			auto const _ = std::unique_lock(token_mutex);
			return mc::ConsumerToken(bubble_service);
		}();

		auto process_requests = [&]
		{
			BubbleRequest request;
			while (bubble_service.try_dequeue(token, request)) {
				tlt.insert_or_update(request.key, close_mapping(request.key));
				for (auto data : request.glob) {
					auto const key = Key(data, 128_u32);
					auto const service = tlt.find(key)->value();
					auto const consumer = service_to_consumer(service, n_services, n_consumers);
					require(consumer < n_consumers);
					while (not queues[consumer].try_enqueue(key)) {
					}
				}
				bubbles_processed += 1;
			}
		};		

		while (not done_producing.test()) {
			process_requests();
		}

		do {
			process_requests();
			cleanup.arrive_and_wait();
		} while (bubbles_processed != bubbles_created);

		require(bubble_service.size_approx() == 0);
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
				auto const node = tlt.find(key);
				require(node != nullptr);
				require(node->has_value());

				auto const service = node->value();
				require(services[service].find(key));

				n += 1;
				m += 1;
			}
		}
		std::print("validated {} tuples\n", m);
	}
}
