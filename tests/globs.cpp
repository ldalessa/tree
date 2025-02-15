#undef DNDEBUG

#include "tree/tree.hpp"

#include <CLI/App.hpp>
#include <mmio/MatrixMarketFile.hpp>

#include <cassert>

using namespace tree;

auto main(int argc, char** argv) -> int
{
	unsigned n_edges = -1u;
	std::string path{};
	
	CLI::App app;
	options::process_command_line(app);
	app.add_option("path", path, "The path to the mmio file")->required();
	app.add_option("n_edges", n_edges, "The number of edges to process");
	CLI11_PARSE(app, argc, app.ensure_utf8(argv));

	options::bubble = 0;
	
	auto const mm = mmio::MatrixMarketFile(path);

	auto tree = GlobTreeNode("0/0");

	for (unsigned n = 0; auto [u, v] : edges(mm)) {
		if (n++ < n_edges) {
			auto const key = Key(std::byteswap((u64)v),std::byteswap((u64)u));
			tree.insert(key);
		}
		else {
			break;
		}
	}
	
	for (unsigned n = 0; auto [u, v] : edges(mm)) {
		if (n++ < n_edges) {
			auto const key = Key(std::byteswap((u64)v),std::byteswap((u64)u));
			assert(tree.find(key));
		}
		else {
			break;
		}
	}
}
