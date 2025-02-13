#include "tree/tree.hpp"

#include <CLI/App.hpp>
#include <mmio/MatrixMarketFile.hpp>
#include <omp.h>

#include <cassert>
#include <format>
#include <print>
#include <vector>

using namespace tree;

auto main(int argc, char** argv) -> int
{
	CLI::App app;
	options::process_command_line(app);

	int n_threads = std::max(omp_get_num_threads(), 2);
	unsigned n_services = 1;
	int n_edges = -1;
	std::string path{};

	app.add_option("path", path, "The path to the mmio file")->required();
	app.add_option("n_edges", n_edges, "The number of edges to process (default: all)");
	app.add_option("-t, --n_threads", n_threads, std::format("The number of threads to use (default: {})", n_threads));
	app.add_option("-n, --n_services", n_services, "The number of services to provision (default: 1)");
	
	CLI11_PARSE(app, argc, app.ensure_utf8(argv));
	
	assert(2 <= n_threads);
	assert(1 <= n_services);
	assert(-1 <= n_edges);

	std::print("n_threads: {}\nn_services: {}\nn_edges: {}\n", n_threads, n_services, n_edges);
	
	omp_set_num_threads(n_threads);
	
	mmio::MatrixMarketFile const mm(path);

	TreeNode<ValueNode<unsigned>> tree("0/0");
	std::vector<SPSCQueue<Key, 1>> queues(n_services);

	std::atomic_flag done = false;
	
#pragma omp parallel
	{
		int i = omp_get_thread_num();

		if (i == 0) {
			int n_service_shift = std::countl_zero((u64)std::bit_ceil(n_services));
			int n = 0;
			for (auto [u, v] : edges(mm)) {
				if (n++ != n_edges) {
					Key key(u,v);
					std::print("pushed: {} {} {} {}\n", i, key, u, v);
					auto service = u >> n_service_shift;
					while (not queues[service].push(key)) {}
				}
			}
			done.test_and_set();
		}
		else {
			unsigned n = 0;
			while (not done.test()) {
				if (auto key = queues[i].pop()) {
					std::print("{} popped: {}\n", i, *key);
					tree.insert(*key, n++);
				}
				std::print("{} queue was empty\n", i);
			}
		}
	}
	
	for (unsigned n = 0; auto [u, v] : edges(mm)) {
		if (n++ < n_edges) {
			auto const node = tree.find(Key(u, v));
			assert(node != nullptr);
			assert(node->get_value());
		}
	}
}
