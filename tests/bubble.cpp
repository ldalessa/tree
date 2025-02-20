#include "tree/tree.hpp"
#include "ingest/mmio.hpp"

#include <CLI/App.hpp>
#include <concurrentqueue.h>

#include <algorithm>
#include <bit>
#include <cassert>
#include <format>
#include <print>
#include <thread>
#include <utility>
#include <vector>

using namespace tree;
namespace mc = moodycamel;

namespace
{
	constexpr auto service_to_consumer(u32 i, u32 n_services, u32 n_consumers) -> u32 {
		auto const d = n_services / n_consumers + !!(n_services % n_consumers);
		return i / d;
	}
	
	constexpr auto tuple_to_key(ingest::Tuple const& tuple) -> Key {
		return Key(tuple.k, tuple.b);
	}
}

auto main(int argc, char** argv) -> int
{
	CLI::App app;
	options::process_command_line(app);

	u32 n_consumers = 1_u32;
	u32 n_producers = 1_u32;
	u32 n_services = 1_u32;
	u32 n_edges = -1_u32;
	bool validate = true;
	std::string path{};

	app.add_option("path", path, "The path to the mmio file")->required();
	app.add_option("n_edges", n_edges, "The number of edges to process (default: all)");
	app.add_option("-c, --n_consumers", n_consumers, std::format("The number of threads to use as consumers (default: {})", n_consumers));
	app.add_option("-p, --n_producers", n_producers, std::format("The number of threads to use as producers (default: {})", n_producers));
	app.add_option("-n, --n_services", n_services, "The number of services to provision (default: 1)");
	app.add_flag("--validate,!--no-validate", validate, "Run the validation code (default: true)");
	
	CLI11_PARSE(app, argc, app.ensure_utf8(argv));
	
	assert(1 <= n_consumers);
	assert(1 <= n_services);

	std::print("n_consumers: {}\nn_producers: {}\nn_services: {}\nn_edges: {}\n", n_consumers, n_producers, n_services, n_edges);	
	std::fflush(stdout);

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
		consumers.emplace_back([&,i=i] {
			u32 n = 0;
			Key key;
			while (not done.test()) {
				while (queues[i].try_dequeue(key)) {
					auto const service = tlt.find(key)->value();
					assert(service_to_consumer(service, n_services, n_consumers) == i);
					assert(services[service].insert(key));
					++n;
				}
			}
			while (queues[i].try_dequeue(key)) {
				auto const service = tlt.find(key)->value();
				assert(service_to_consumer(service, n_services, n_consumers) == i);
				assert(services[service].insert(key));
				++n;
			}

			std::print("consumer {} processed {} keys\n", i, n);
		});
	}

	// Start the producer threads.
	for (u32 i = 0; i < n_producers; ++i) {
		producers.emplace_back([&,i=i] {
			auto mm = ingest::mmio::Reader(path, n_producers, i);
			int n_consumer_shift = std::countl_zero((u64)std::bit_ceil(n_consumers)) + 1;
			u32 n = 0;
			while (auto tuple = mm.next()) {
				if (n++ < n_edges) {
					auto const key = tuple_to_key(*tuple);
					auto const service = tlt.find(key)->value();
					auto const consumer = service_to_consumer(service, n_services, n_consumers);
					assert(consumer < n_consumers);
					queues[consumer].enqueue(key);
				}
			}
			std::print("producer {} processed {} keys\n", i, n);
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
		u32 n = 0;
		while ( auto tuple = mm.next()) {
			if (n++ < n_edges) {
				auto const key = tuple_to_key(*tuple);
				auto const node = tlt.find(key);
				assert(node != nullptr);
				assert(node->has_value());
			}
		}
	}
}
