#include <CLI/App.hpp>
#include <mmio/MatrixMarketFile.hpp>
#include <cassert>

import tree;
import std;

using namespace tree;

auto main(int argc, char** argv) -> int
{
	CLI::App app;
	options::process_command_line(app);

	unsigned n_threads = 1;
	unsigned n_edges = -1;
	std::string path{};

	app.add_option("path", path, "The path to the mmio file")->required();
	app.add_option("n_edges", n_edges, "The number of edges to process (default: all)");
	app.add_option("--n_threads", n_threads, "The number of threads to use (default: 1)");
	
	CLI11_PARSE(app, argc, app.ensure_utf8(argv));

	mmio::MatrixMarketFile const mm(path);

	TreeNode<ValueNode<unsigned>> tree("0/0");

	std::vector<std::jthread> threads;

	for (int i = 0; i < n_threads; ++i) {
		threads.emplace_back([&tree, &mm, n_edges] {
			for (unsigned n = 0; auto [u, v] : edges(mm)) {
				if (n++ < n_edges) {
					tree.insert(Key(u, v), n);
				}
			}
		});
	}
	
	for (auto& thread : threads) {
		thread.join();
	}
	
	for (unsigned n = 0; auto [u, v] : edges(mm)) {
		if (n++ < n_edges) {
			auto [a, b] = tree.find(Key(u, v), nullptr);
			assert(a != nullptr);
			assert(b != nullptr);
			assert(a == b);
			assert(a->get_value() and *a->get_value() == n);
		}
	}
}
