#include "die/uses.hpp"
#include "die/utils.hpp"

auto
die::uses::print_tuple(Tuple const& t)
    -> std::string
{
    return std::format("{},{}", print_id(t.k), print_id(t.b));
}
