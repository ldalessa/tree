#include "tree/tree.hpp"

#include <CLI/App.hpp>
#include <mmio/MatrixMarketFile.hpp>

#include <cassert>
#include <format>
#include <print>
#include <thread>
#include <vector>

using namespace tree;

auto main(int argc, char** argv) -> int
{
	CLI::App app;
	options::process_command_line(app);

	int n_threads = 2;
	unsigned n_services = 1;
	unsigned n_edges = -1;
	std::string path{};

	app.add_option("path", path, "The path to the mmio file")->required();
	app.add_option("n_edges", n_edges, "The number of edges to process (default: all)");
	app.add_option("-t, --n_threads", n_threads, std::format("The number of threads to use (default: {})", n_threads));
	app.add_option("-n, --n_services", n_services, "The number of services to provision (default: 1)");
	
	CLI11_PARSE(app, argc, app.ensure_utf8(argv));
	
	assert(2 <= n_threads);
	assert(1 <= n_services);

	std::print("n_threads: {}\nn_services: {}\nn_edges: {}\n", n_threads, n_services, n_edges);	
	std::fflush(stdout);
	
	auto const mm = mmio::MatrixMarketFile(path);

	auto tree = TreeNode<ValueNode<unsigned>>("0/0");
	auto queues = std::vector<SPSCQueue<Key, 1>>(n_services);
	auto done = std::atomic_flag(false);
	auto consumers = std::vector<std::jthread>();

	for (int i = 0; i < n_threads - 1; ++i) {
		consumers.emplace_back([i,&tree,&queues,&done] {
			unsigned n = 0;
			while (not done.test()) {
				while (auto key = queues[i].pop()) {
					tree.insert(*key, n++);
				}
			}
			while (auto key = queues[i].pop()) {
				tree.insert(*key, n++);
			}
		});
	}
	
	int n_service_shift = std::countl_zero((u64)std::bit_ceil(n_services));
	int n = 0;
	for (auto [u, v] : edges(mm)) {
		if (n++ < n_edges) {
			Key key(u,v);
			auto service = u >> n_service_shift;
			while (not queues[service].push(key)) {
			}
		}
	}
	done.test_and_set();

	for (auto& thread : consumers) {
		thread.join();
	}
	
	for (unsigned n = 0; auto [u, v] : edges(mm)) {
		if (n++ < n_edges) {
			auto const node = tree.find(Key(u, v));
			assert(node != nullptr);
			assert(node->get_value());
		}
	}
}
