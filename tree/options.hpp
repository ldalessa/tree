#pragma once

#include "tree/types.hpp"

#include <CLI/CLI.hpp>
#include <cassert>
#include <print>

namespace tree::options
{
	inline u32 verbose = 0;
	inline u32 debug = 0;

	inline void set_verbose(i64 count)
	{
		assert(0 < count and count < (u32)-1);
		verbose = count;
	}

	inline void set_debug(i64 count)
	{
		assert(0 < count and count < (u32)-1);
		debug = count;
	}

	inline void process_command_line(CLI::App& app)
	{
		app.add_flag_function("-v,--verbose", set_verbose, "Generate verbose output");
		app.add_flag_function("-d,--debug", set_debug, "Run debug code and generate debug output");
	}

	inline void print_options(FILE* out) {
		std::print(out, "options::verbose: {}\n", tree::options::verbose);
		std::print(out, "options::debug: {}\n", tree::options::debug);
	}
}
