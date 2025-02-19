#pragma once

#include "ingest/FileReader.hpp"
#include "ingest/Tuple.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace die::wmd
{
    /// The different types of tuples for the wmd input.
    inline
    namespace tuple_types
    {
        static constexpr std::uint64_t _None = 0;
        static constexpr std::uint64_t _Author_ForumEvent = 1;
        static constexpr std::uint64_t _Author_Publication = 2;
        static constexpr std::uint64_t _Forum = 3;
        static constexpr std::uint64_t _ForumEvent = 4;
        static constexpr std::uint64_t _HasOrg = 5;
        static constexpr std::uint64_t _HasTopic_Forum = 6;
        static constexpr std::uint64_t _HasTopic_ForumEvent = 7;
        static constexpr std::uint64_t _HasTopic_Publication = 8;
        static constexpr std::uint64_t _Includes = 9;
        static constexpr std::uint64_t _Person = 10;
        static constexpr std::uint64_t _Publication = 11;
        static constexpr std::uint64_t _Sale = 12;
        static constexpr std::uint64_t _Topic = 13;
    }

    /// Parse a tuple from a csv string.
    auto parse_tuple(char const*, char const*) -> std::optional<Tuple>;
    auto parse_tuple(std::string_view) -> std::optional<Tuple>;

    /// Print a tuple to a csv string.
    auto print_as_csv(Tuple const&) -> std::string;

    /// Specialize the file reader for wmd tuples.
    struct Reader : FileReader<Reader>
    {
        using FileReader::FileReader;

        auto _parse_tuple(char const* i, char const* e) const -> std::optional<Tuple>
        {
            return parse_tuple(i, e);
        }
    };

    /// The tuple type is stored in the .type tuple field.
    inline auto type(Tuple const& tuple) -> std::uint64_t {
        return tuple.type;
    }
}

