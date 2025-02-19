#include "die/utils.hpp"
#include "die/coffee.hpp"
#include "die/commercial.hpp"

auto
die::commercial::parse_tuple(char const* str, char const* e)
    -> std::optional<Tuple>
{
    if (str[0] == '#') {
        return std::nullopt;
    }

    /// Sale,1097627,1430378,271997,2/17/2019,,,
    /// Sale,1224191,1224191,8486,,,,87269.0090988878

    auto const seller = parse_id(skip(str, "Sale,"), e);
    auto const buyer = parse_id(skip(str, ","), e);
    auto const product = maybe_parse_int(skip(str, ","), e);

    if (str[2] == ',') {
        return Tuple {
            .k = seller,
            .type = coffee::tuple_types::SaleAmount,
            .b = buyer,
            .c = product,
            .d = parse_double(skip(str, ",,,,"), e)
        };
    }

    return Tuple {
        .k = seller,
        .type = coffee::tuple_types::Sale,
        .b = buyer,
        .c = product,
        .d = parse_date(skip(str, ","), e)
    };
}

auto
die::commercial::parse_tuple(std::string_view str)
    -> std::optional<Tuple>
{
    return parse_tuple(str.begin(), str.end());
}

