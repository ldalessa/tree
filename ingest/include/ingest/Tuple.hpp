#pragma once

#include <compare>
#include <cstdint>
#include <format>
#include <ranges>
#include <string_view>

namespace ingest
{
    /// A basic uniform tuple structure used across the different ingest
    /// algorithms.
    struct Tuple
    {
        std::uint64_t k = 0;
        std::uint64_t type = 0;
        std::uint64_t b = 0;
        std::uint64_t c = 0;
        std::uint64_t d = 0;

        friend auto operator==(Tuple const&, Tuple const&) -> bool = default;
        friend auto operator<=>(Tuple const&, Tuple const&) = default;

		constexpr auto to_key() const -> unsigned __int128 {
			return ((unsigned __int128)(k) << 64) | b;
		}
		
		constexpr operator unsigned __int128() const {
			return to_key();
		}
    };
}

template <>
struct std::formatter<ingest::Tuple>
{
	std::string_view _key_fmt{};
	
	constexpr auto parse(auto&& ctx) -> decltype(ctx.begin()) {
		auto i = ctx.begin();
		auto j = ranges::find(i, ctx.end(), '}');
		_key_fmt = std::string_view(i, j);
		return j;
	}

	constexpr auto format(ingest::Tuple const& a, auto&& ctx) const -> decltype(ctx.out()) {
		return std::format_to(ctx.out(), "key:{{}} {{{}, {}, {}, {}, {}}}", a.to_key(), _key_fmt, a.k, a.type, a.b, a.c, a.d);
	}
};
