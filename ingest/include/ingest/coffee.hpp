#pragma once

#include "ingest/FileReader.hpp"
#include "ingest/Tuple.hpp"
#include <cstdint>
#include <optional>
#include <string_view>

namespace ingest::coffee
{
    inline
    namespace tuple_types
    {
        inline constexpr std::uint64_t None = 0;
        inline constexpr std::uint64_t Cyber = 1;
        inline constexpr std::uint64_t Social = 2;
        inline constexpr std::uint64_t Uses = 3;
        inline constexpr std::uint64_t Sale = 4;
        inline constexpr std::uint64_t SaleAmount = 5;
        inline constexpr std::uint64_t Wsales = 6; // not really ingested
    }

    enum class FileType {
        NONE = 0,
        COMMERCIAL,
        CYBER,
        SOCIAL,
        USES,
        COUNT
    };

    auto parse_tuple(char const*, char const*, FileType) -> std::optional<Tuple>;
    auto parse_tuple(std::string_view, FileType) -> std::optional<Tuple>;

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
