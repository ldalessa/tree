#include "tree/tree.hpp"

#include <CLI/App.hpp>
#include <mmio/MatrixMarketFile.hpp>
#include <omp.h>

#include <cassert>

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

	omp_set_num_threads(n_threads);
	
	mmio::MatrixMarketFile const mm(path);

	TreeNode<ValueNode<unsigned>> tree("0/0");


#pragma omp parallel
	for (unsigned n = 0; auto [u, v] : edges(mm)) {
		if (n++ < n_edges) {
			tree.insert(Key(u, v), n);
		}
	}
	
	for (unsigned n = 0; auto [u, v] : edges(mm)) {
		if (n++ < n_edges) {
			auto const node = tree.find(Key(u, v));
			assert(node != nullptr);
			assert(node->get_value() and *node->get_value() == n);
		}
	}
}
