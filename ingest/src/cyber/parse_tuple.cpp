#include "die/utils.hpp"
#include "die/coffee.hpp"
#include "die/cyber.hpp"

auto
die::cyber::parse_tuple(char const* str, char const* e)
    -> std::optional<Tuple>
{
    if (str[0] == '#') {
        return std::nullopt;
    }

    // ignore most of the data
    return Tuple {
        .k = parse_id(str, e),                  // src_device
        .type = coffee::tuple_types::Cyber,
        .b = parse_id(skip(str, ","), e)        // dst_device
    };
}

auto
die::cyber::parse_tuple(std::string_view str)
    -> std::optional<Tuple>
{
    return parse_tuple(str.begin(), str.end());
}

