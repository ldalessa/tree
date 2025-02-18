#pragma once

#include "tree/types.hpp"

#include <CLI/CLI.hpp>
#include <cassert>
#include <format>
#include <print>
#include <utility>

namespace tree::options
{
	inline Fit local_fit = BEST;
	inline Fit global_fit = BEST;
	inline u64 default_glob_capacity = 256;
	inline u32 factor = 64;
	inline u32 bubble = -1_u32;
	inline u32 verbose = 0;
	inline u32 debug = 0;

	
	inline void print_options(FILE* out)
	{
		std::print(out, "options::local-fit: {}\n", fit_to_string(local_fit));
		std::print(out, "options::global-fit: {}\n", fit_to_string(global_fit));
		std::print(out, "options::default-glob-capacity: {}\n", default_glob_capacity);
		std::print(out, "options::factor: {}\n", factor);
		std::print(out, "options::bubble: {}\n", bubble);
		std::print(out, "options::verbose: {}\n", verbose);
		std::print(out, "options::debug: {}\n", debug);
	}
	
	namespace cli
	{
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

		auto const check_fit = CLI::Validator([](std::string& str) -> std::string {
			try {
				auto const fit = string_to_fit(str);
				str = std::format("{}", std::to_underlying(fit));
				return "";
			} catch(std::runtime_error const& e) {
				return e.what();
			}
		}, "Fit", "check that a string is a valid Fit");
	
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
	}
	
	inline void process_command_line(CLI::App& app)
	{
		cli::add_flag(app, "-v,--verbose", cli::set_verbose, "Generate verbose output");
		cli::add_flag(app, "-d,--debug", cli::set_debug, "Run debug code and generate debug output");
		cli::add_option(app, "-m,--default-glob-capacity", default_glob_capacity, "The default glob capacity");	
		cli::add_option(app, "--factor", factor, "The mask at which to factor");
		cli::add_option(app, "--bubble", bubble, "The mask at which to bubble");

		app.add_option("-l,--local-fit", local_fit, "The local fit algorithm {best,first} (default: best)")->transform(cli::check_fit);
		app.add_option("-g,--global-fit", global_fit, "The global fit algorithm {best,first} (default: best)")->transform(cli::check_fit);
	}
}
