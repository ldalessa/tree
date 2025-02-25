#pragma once

#include <compare>
#include <cstdint>
#include <format>

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
	static constexpr auto parse(auto& ctx) -> decltype(ctx.begin()) {
		return ctx.begin();
	}

	static constexpr auto format(ingest::Tuple const& a, auto& ctx) {
		return std::format_to(ctx.out(), "key:{:032x} {{{}, {}, {}, {}, {}}}", a.to_key(), a.k, a.type, a.b, a.c, a.d);
	}
};
