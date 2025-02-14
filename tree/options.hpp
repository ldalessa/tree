#pragma once

#include "tree/types.hpp"

#include <CLI/CLI.hpp>
#include <cassert>
#include <format>
#include <print>
#include <utility>

namespace tree::options
{
	inline u64 default_glob_capacity = 256;
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

	template <class T, class... Args>
	inline auto add_flag(CLI::App& app, char const* opt, void (*f)(T), std::format_string<Args...> fmt, Args&&... args)
		-> decltype(auto)
	{
		app.add_flag_function(opt, f, std::format(fmt, std::forward<Args>(args)...));
	}

	template <class... Args>
	inline auto add_option(CLI::App& app, char const* opt, auto& v, std::format_string<Args...> fmt, Args&&... args)
		-> decltype(auto)
	{
		app.add_option(opt, v, std::format("{} (default: {})", std::format(fmt, std::forward<Args>(args)...), v));
	}
	
	inline void process_command_line(CLI::App& app)
	{
		add_flag(app, "-v,--verbose", set_verbose, "Generate verbose output");
		add_flag(app, "-d,--debug", set_debug, "Run debug code and generate debug output");
		add_option(app, "-m,--default-glob-capacity", default_glob_capacity, "The default glob capacity");	
	}

	inline void print_options(FILE* out) {
		std::print(out, "options::default-glob-capacity: {}\n", tree::options::default_glob_capacity);
		std::print(out, "options::verbose: {}\n", tree::options::verbose);
		std::print(out, "options::debug: {}\n", tree::options::debug);
	}
}
