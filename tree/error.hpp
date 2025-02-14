#pragma once

#include <format>
#include <stdexcept>
#include <utility>

namespace tree
{
	template <class... Args>
	auto error(std::format_string<Args...> fmt, Args&&... args) {
		return std::runtime_error(std::format(fmt, std::forward<Args>(args)...));
	}
}
