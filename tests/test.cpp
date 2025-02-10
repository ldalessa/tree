#include <CLI/App.hpp>
#include <cassert>

import tree;
import std;

using namespace tree;

int main(int argc, char* argv[]) {
	CLI::App app;
	argv = app.ensure_utf8(argv);
	options::process_command_line(app);
	CLI11_PARSE(app, argc, argv);

	options::print_options(stdout);

	testing::run_all_tests();
	
	// Key k("1/1");

	// std::print("{}\n", k);

	// {
	// 	Key a("1/1");
	// 	assert((a ^ a) == a);
	// }
	
	// {
	// 	TreeNode<int> node(k);
	// 	{
	// 		auto [a, b] = node.find(k, nullptr);
	// 		assert(not a);
	// 		assert(b and b->key() == k);
	// 	}
	// 	auto& next = node.insert("1/1", 0);
	// 	{
	// 		auto [a, b] = node.find(k, nullptr);
	// 		assert(a);
	// 		assert(a and a->key() == k);
	// 	}
	// }

	// {
	// 	TreeNode<int> node("0/0");
	// 	node.insert("0/128", 0);
	// 	node.insert("1/128", 1);
	// 	node.insert("0/127", 2);
	// }
}
