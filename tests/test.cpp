#define TREE_TESTING

#include "tree/tree.hpp"

#include <CLI/App.hpp>

int main(int argc, char* argv[]) {
	CLI::App app;
	argv = app.ensure_utf8(argv);
	tree::options::process_command_line(app);
	CLI11_PARSE(app, argc, argv);
	tree::testing::run_all_tests();
}
