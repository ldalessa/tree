#include "tree/tree.hpp"
#include "ingest/mmio.hpp"

#include <CLI/App.hpp>

#include <cassert>
#include <print>

using namespace tree;
using namespace tree::tests;

auto main(int argc, char** argv) -> int
{
	u32 n_edges = -1_u32;
	std::string path{};
	
	CLI::App app;
	options::process_command_line(app);
	app.add_option("path", path, "The path to the mmio file")->required();
	app.add_option("n_edges", n_edges, "The number of edges to process");
	CLI11_PARSE(app, argc, app.ensure_utf8(argv));

	if (options::bubble != -1_u32) {
		std::print("suppressing bubbling in this test\n");
		options::bubble = -1_u32;
	}

	auto tree = GlobTreeNode<ingest::Tuple>("0/0");

	{
		auto mm = ingest::mmio::Reader(path);
		u32 n = 0_u32;
		while (auto tuple = mm.next()) {
			if (n++ < n_edges) {
				tree.insert(*tuple);
			}
			else {
				break;
			}
		}
	}

	{
		auto mm = ingest::mmio::Reader(path);
		u32 n = 0_u32;
		while (auto tuple = mm.next()) {
			if (n++ < n_edges) {
				assert(tree.find(*tuple));
			}
			else {
				break;
			}
		}
	}
}
