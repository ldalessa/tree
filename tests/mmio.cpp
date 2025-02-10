#include <CLI/App.hpp>

import tree;

using namespace tree;

auto main(int argc, char** argv) -> int
{
  CLI::App app;
  options::process_command_line(app);
  CLI11_PARSE(app, argc, app.ensure_utf8(argv));
}
