#include "die/utils.hpp"
#include "die/edge_list.hpp"
#include <format>

using die::Tuple;
using namespace die::utils;
using namespace die::edge_list::tuple_types;


auto
die::edge_list::parse_tuple(char const* str, char const* e, FileType type)
    -> std::optional<Tuple>
{
    if (str[0] == '#') {
        return std::nullopt;
    }

    if(type == die::edge_list::FileType::WEIGHT){
        return Tuple {
            .k = parse_id(str, e),
            .type = edge_list::tuple_types::_Weight,
            .b = parse_id(skip(str, " "), e),
            .c = parse_int(skip(str, " "), e)
        };
    }

    return Tuple {
        .k = parse_id(str, e),
        .type = edge_list::tuple_types::_No_weight,
        .b = parse_id(skip(str, " "), e),
        .c = parse_int(skip(str, " "), e)
    };
}

auto
die::edge_list::parse_tuple(std::string_view str, FileType type)
    -> std::optional<Tuple>
{
    return parse_tuple(str.begin(), str.end(), type);
}
