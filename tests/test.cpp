#define TREE_TESTING
#undef DNDEBUG

#include "tree/tree.hpp"
#include <CLI/App.hpp>

using namespace tree;

int main(int argc, char* argv[]) {
	CLI::App app;
	argv = app.ensure_utf8(argv);
	options::process_command_line(app);
	CLI11_PARSE(app, argc, argv);
	testing::run_all_tests();
}
