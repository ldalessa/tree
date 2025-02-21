#include "require.hpp"

#include "tree/tree.hpp"
#include "ingest/mmio.hpp"

#include <CLI/App.hpp>
#include <concurrentqueue.h>

#include <algorithm>
#include <bit>
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

auto main(int argc, char** argv) -> int
{
	CLI::App app;
	options::process_command_line(app);

	u32 n_consumers = 1_u32;
	u32 n_producers = 1_u32;
	u32 n_services = 1_u32;
	u64 n_edges = -1_u64;
	bool validate = true;
	std::string path{};

	app.add_option("path", path, "The path to the mmio file")->required();
	app.add_option("n_edges", n_edges, "The number of edges to process (default: all)");
	app.add_option("-c, --n_consumers", n_consumers, std::format("The number of threads to use as consumers (default: {})", n_consumers));
	app.add_option("-p, --n_producers", n_producers, std::format("The number of threads to use as producers (default: {})", n_producers));
	app.add_option("-n, --n_services", n_services, "The number of services to provision (default: 1)");
	app.add_flag("--validate,!--no-validate", validate, "Run the validation code (default: true)");
	
	CLI11_PARSE(app, argc, app.ensure_utf8(argv));
	
	require(1 <= n_consumers);
	require(1 <= n_services);

	std::print("n_consumers: {}\nn_producers: {}\nn_services: {}\nn_edges: {}\n", n_consumers, n_producers, n_services, n_edges);	
	std::fflush(stdout);

	// auto tlt = TreeNode<u32>("0/0");
	auto tlt = TopLevelTreeNode("0/0");
	auto queues = std::vector<mc::ConcurrentQueue<Key>>(n_consumers);
	auto done = std::atomic_flag(false);
	auto producers = std::vector<std::jthread>();
	auto consumers = std::vector<std::jthread>();
	auto services = std::vector<GlobTreeNode>();

	// Allocate local trees for each service.
	services.reserve(n_services);
	for (u32 i = 0; i < n_services; ++i) {
		services.emplace_back("0/0");
	}

	// Allocate top level tree nodes for each service.
	for (u32 i = 0; i < n_services; ++i) {
		u32 const n_bits = std::countr_zero(std::bit_ceil(n_services));
		tlt.insert_or_update({std::rotr((u128)i, n_bits), n_bits}, i);
	}

	// Start the consumer threads.
	for (u32 i = 0; i < n_consumers; ++i) {
		consumers.emplace_back([&,i=i]
		{
			std::print("starting consumer {}\n", i);
			
			auto token = mc::ConsumerToken(queues[i]);
			
			u64 n = 0;
			u64 stalls = 0;
			
			Key key;
			while (not done.test()) {
				while (queues[i].try_dequeue(token, key)) {
					auto const service = tlt.find(key)->value();
					require(service_to_consumer(service, n_services, n_consumers) == i);
					require(services[service].insert(key));
					n += 1;
				}
				
				stalls += 1;
			}
			while (queues[i].try_dequeue(token, key)) {
				auto const service = tlt.find(key)->value();
				require(service_to_consumer(service, n_services, n_consumers) == i);
				require(services[service].insert(key));
				n += 1;
			}

			std::print("consumer {} processed {} keys ({} stalls)\n", i, n, stalls);
		});
	}

	// Start the producer threads.
	for (u32 i = 0; i < n_producers; ++i) {
		producers.emplace_back([&,i=i]
		{
			std::print("starting producer {}\n", i);
			
			// Allocate a token for each of the queues.
			std::vector<mc::ProducerToken> tokens;
			tokens.reserve(n_consumers);
			for (auto& q : queues) {
				tokens.emplace_back(q);
			}

			// Open the file for reading.
			auto mm = ingest::mmio::Reader(path, n_producers, i);			

			// Count the number of tuples that I'm processing, and the amount of
			// stalling I have to do.
			u64 n = 0;
			u64 stalls = 0;
			// Process each tuple.
			while (auto tuple = mm.next()) {
				if (n == n_edges) break;
				
				auto const key = tuple_to_key(*tuple);
				auto const service = tlt.find(key)->value();
				auto const consumer = service_to_consumer(service, n_services, n_consumers);
				require(consumer < n_consumers);
				while (not queues[consumer].try_enqueue(tokens[consumer], key)) {
					stalls += 1;
				}
				n += 1;
			}
			
			// Dump output for this thread.
			std::print("producer {} processed {} keys ({} stalls)\n", i, n, stalls);
		});
	}

	// Wait for the producers to complete.
	for (auto& thread : producers) {
		thread.join();
	}

	// Signal the consumers.
	done.test_and_set();

	// Wait for the consumers to complete.
	for (auto& thread : consumers) {
		thread.join();
	}

	if (validate) {
		auto mm = ingest::mmio::Reader(path);
		u64 n = 0;
		while ( auto tuple = mm.next()) {
			if (n++ < n_edges) {
				auto const key = tuple_to_key(*tuple);
				auto const node = tlt.find(key);
				require(node != nullptr);
				require(node->has_value());
				require(node->key() == key);
			}
		}
		std::print("validated {} tuples\n", n);
	}
}
