#pragma once

#include <compare>
#include <cstdint>

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
    };
}
