#include "die/coffee.hpp"
#include "die/commercial.hpp"
#include "die/utils.hpp"
#include <cassert>

auto
die::commercial::print_tuple(Tuple const& t)
    -> std::string
{
    if (t.type == coffee::tuple_types::Sale) {
        return std::format("Sale,{},{},{},{},,,", print_id(t.k), print_id(t.b), maybe_print_int(t.c), print_date(t.d));
    }
    assert(t.type == coffee::tuple_types::SaleAmount);
    return std::format("Sale,{},{},{},,,,{}", print_id(t.k), print_id(t.b), maybe_print_int(t.c), print_double(t.d));
}
