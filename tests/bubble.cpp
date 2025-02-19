#include "tree/tree.hpp"

#include <CLI/App.hpp>
#include <mmio/MatrixMarketFile.hpp>

#include <algorithm>
#include <bit>
#include <cassert>
#include <format>
#include <print>
#include <thread>
#include <utility>
#include <vector>

using namespace tree;

namespace
{
	constexpr auto service_to_consumer(u32 i, u32 n_services, u32 n_consumers) -> u32 {
		auto const d = n_services / n_consumers + !!(n_services % n_consumers);
		return i / d;
	}
	
	constexpr auto edge_to_key(auto const& edge) -> Key {
		auto const& [u, v] = edge;
		return Key(std::byteswap((u64)u), std::byteswap((u64)v));
	}
}

auto main(int argc, char** argv) -> int
{
	CLI::App app;
	options::process_command_line(app);

	u32 n_consumers = 1;
	u32 n_services = 1;
	u32 n_edges = -1;
	bool validate = true;
	std::string path{};

	app.add_option("path", path, "The path to the mmio file")->required();
	app.add_option("n_edges", n_edges, "The number of edges to process (default: all)");
	app.add_option("-c, --n_consumers", n_consumers, std::format("The number of threads to use as consumers (default: {})", n_consumers));
	app.add_option("-n, --n_services", n_services, "The number of services to provision (default: 1)");
	app.add_flag("--validate,!--no-validate", validate, "Run the validation code (default: true)");
	
	CLI11_PARSE(app, argc, app.ensure_utf8(argv));
	
	assert(1 <= n_consumers);
	assert(1 <= n_services);

	std::print("n_consumers: {}\nn_services: {}\nn_edges: {}\n", n_consumers, n_services, n_edges);	
	std::fflush(stdout);
	
	auto const mm = mmio::MatrixMarketFile(path);

	auto tlt = TopLevelTreeNode("0/0");
	auto queues = std::vector<SPSCQueue<Key, 4096>>(n_consumers);
	auto done = std::atomic_flag(false);
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
			while (not done.test()) {
				while (auto key = queues[i].pop()) {
					auto const service = tlt.find(*key)->value();
					assert(service_to_consumer(service, n_services, n_consumers) == i);
					assert(services[service].insert(*key));
					++n;
				}
			}
			while (auto key = queues[i].pop()) {
				auto const service = tlt.find(*key)->value();
				assert(service_to_consumer(service, n_services, n_consumers) == i);
				assert(services[service].insert(*key));
				++n;
			}

			std::print("consumer {} processed {} keys\n", i, n);
		});
	}

	// Become the producer thread.
	int n_consumer_shift = std::countl_zero((u64)std::bit_ceil(n_consumers)) + 1;
	int n = 0;
	std::print("consumer shift: {}\n", n_consumer_shift);
	for (auto const& edge : edges(mm)) {
		if (n++ < n_edges) {
			auto const key = edge_to_key(edge);
			auto const service = tlt.find(key)->value();
			auto const consumer = service_to_consumer(service, n_services, n_consumers);
			assert(consumer < n_consumers);
			while (not queues[consumer].push(key)) {
			}
		}
	}
	done.test_and_set();

	for (auto& thread : consumers) {
		thread.join();
	}

	if (not validate) {
		return EXIT_SUCCESS;
	}
	
	for (u32 n = 0; auto const& edge : edges(mm)) {
		if (n++ < n_edges) {
			auto const key = edge_to_key(edge);
			auto const node = tlt.find(key);
			assert(node != nullptr);
			assert(node->has_value());
		}
	}
}
