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

	unsigned N = -1;
	std::string path{};

	app.add_option("path", path, "The path to the mmio file")->required();
	app.add_option("n_edges", N, "The number of edges to process (default=all)");
  
	CLI11_PARSE(app, argc, app.ensure_utf8(argv));

	mmio::MatrixMarketFile mm(path);

	TreeNode<ValueNode<unsigned>> tree("0/0");
	
	for (unsigned n = 0; auto [u, v] : edges(mm)) {
		if (n++ < N) {
			tree.insert(Key(u, v), n);
		}
	}

	for (unsigned n = 0; auto [u, v] : edges(mm)) {
		if (n++ < N) {
			auto [a, b] = tree.find(Key(u, v), nullptr);
			assert(a != nullptr);
			assert(b != nullptr);
			assert(a == b);
			assert(a->get_value() and *a->get_value() == n);
		}
	}
}
