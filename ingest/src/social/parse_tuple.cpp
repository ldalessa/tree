#include "die/utils.hpp"
#include "die/coffee.hpp"
#include "die/social.hpp"

auto
die::social::parse_tuple(char const* str, char const* e)
    -> std::optional<Tuple>
{
    if (str[0] == '#') {
        return std::nullopt;
    }

    return Tuple {
        .k = parse_id(str, e),
        .type = coffee::tuple_types::Social,
        .b = parse_id(skip(str, ","), e)
    };
}

auto
die::social::parse_tuple(std::string_view str)
    -> std::optional<Tuple>
{
    return parse_tuple(str.begin(), str.end());
}

