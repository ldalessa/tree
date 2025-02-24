#include "require.hpp"
#include "MPSCQueue.hpp"
#include "tree/tree.hpp"
#include "ingest/mmio.hpp"

#include <CLI/App.hpp>

#include <bit>
#include <format>
#include <print>
#include <thread>
#include <vector>

using namespace tree;

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

	u32 n_consumers = 1;
	u32 n_services = 1;
	u64 n_edges = -1;
	bool validate = true;
	std::string path{};

	app.add_option("path", path, "The path to the mmio file")->required();
	app.add_option("n_edges", n_edges, "The number of edges to process (default: all)");
	app.add_option("-c, --n_consumers", n_consumers, std::format("The number of threads to use as consumers (default: {})", n_consumers));
	app.add_option("-n, --n_services", n_services, "The number of services to provision (default: 1)");
	app.add_flag("--validate,!--no-validate", validate, "Run the validation code (default: true)");
	
	CLI11_PARSE(app, argc, app.ensure_utf8(argv));
	
	require(1 <= n_consumers);
	require(1 <= n_services);

	std::print("n_consumers: {}\nn_services: {}\nn_edges: {}\n", n_consumers, n_services, n_edges);	
	std::fflush(stdout);

	auto tree = TreeNode<u32>("0/0");
	auto queues = std::vector<SPSCQueue<Key, 512>>(n_consumers);
	auto done = std::atomic_flag(false);
	auto consumers = std::vector<std::jthread>();

	for (int i = 0; i < n_consumers; ++i) {
		consumers.emplace_back([i,&tree,&queues,&done]
		{
			u64 n = 0;
			u64 stalls = 0;
			
			while (not done.test()) {
				while (auto key = queues[i].pop()) {
					tree.insert_or_update(*key, n++);
				}
				stalls += 1;
			}
			while (auto key = queues[i].pop()) {
				tree.insert_or_update(*key, n++);
			}

			std::print("consumer {} processed {} keys ({} stalls)\n", i, n, stalls);
		});
	}

	{
		auto mm = ingest::mmio::Reader(path);
		int n_bits = std::countr_zero((u64)std::bit_ceil(n_consumers));
		u64 n = 0;

		while (auto tuple = mm.next()) {
			if (n == n_edges) break;
			
			auto const key = tuple_to_key(*tuple);
			auto const service = std::rotl(key.data(), n_bits);
			auto const consumer = service_to_consumer(service, n_services, n_consumers);
			while (not queues[consumer].push(key)) {
			}
			n += 1;
		}
		done.test_and_set();
	}
	
	for (auto& thread : consumers) {
		thread.join();
	}

	if (validate) {
		auto mm = ingest::mmio::Reader(path);
		u32 n = 0;		
		while (auto tuple = mm.next()) {
			if (n++ < n_edges) {
				auto const key = tuple_to_key(*tuple);
				auto const node = tree.find(key);
				require(node != nullptr);
				require(node->has_value());
				require(node->key() == key);
			}
		}
	}
}
