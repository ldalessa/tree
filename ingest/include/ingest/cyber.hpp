#pragma once

#include "ingest/FileReader.hpp"
#include "ingest/Tuple.hpp"

#include <cstdint>
#include <optional>
#include <string_view>

namespace ingest::cyber
{
    auto parse_tuple(char const*, char const*) -> std::optional<Tuple>;
    auto parse_tuple(std::string_view) -> std::optional<Tuple>;

    struct Reader : FileReader<Reader>
    {
        using FileReader::FileReader;

        auto _parse_tuple(char const* i, char const* e) const -> std::optional<Tuple> {
            return parse_tuple(i, e);
        }
    };
}
