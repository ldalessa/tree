#include <CLI/App.hpp>

import tree;

int main(int argc, char* argv[]) {
	CLI::App app;
	argv = app.ensure_utf8(argv);
	tree::options::process_command_line(app);
	CLI11_PARSE(app, argc, argv);
	tree::testing::run_all_tests();
}
