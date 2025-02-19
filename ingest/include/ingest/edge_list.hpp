#pragma once

#include "ingest/FileReader.hpp"
#include "ingest/Tuple.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace ingest::edge_list
{
    /// The different types of tuples for the wmd input.
    inline
    namespace tuple_types
    {
        static constexpr std::uint64_t _None = 0;
        static constexpr std::uint64_t _Weight = 1;
        static constexpr std::uint64_t _No_weight = 2;
    }

    enum class FileType {
        NONE = 0,
        WEIGHT,
        NO_WEIGHT,
        COUNT
    };

    /// Parse a tuple from a csv string.
    auto parse_tuple(char const*, char const*, FileType) -> std::optional<Tuple>;
    auto parse_tuple(std::string_view, FileType) -> std::optional<Tuple>;

    /// Predict the number of cycles that parsing a tuple type takes.
    // auto parse_cycles(Tuple const&, splitmix32&) -> std::uint32_t;

    /// Print a tuple to a csv string.
    auto print_as_csv(Tuple const&) -> std::string;

    /// Specialize the file reader for wmd tuples.
    struct Reader : FileReader<Reader>
    {
        FileType _type{};

        Reader(FileType type, auto&&... args)
                : FileReader(args...)
                , _type(type)
        {
        }

        void set_file_type(FileType type) {
            _type = type;
        }

        auto _parse_tuple(char const* i, char const* e) const -> std::optional<Tuple>
        {
            return parse_tuple(i, e, _type);
        }
    };
}
