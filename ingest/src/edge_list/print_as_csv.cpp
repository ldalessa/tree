#include "die/Tuple.hpp"
#include "die/utils.hpp"
#include "die/edge_list.hpp"

#include <exception>
#include <format>

using die::Tuple;
using die::utils::print_int;
using die::utils::maybe_print_id;
using die::utils::print_date;
using die::utils::print_id;


static auto _print_weight_edge(Tuple const& tuple) -> std::string
{
    return std::format("{},{},{}", print_id(tuple.k), print_id(tuple.b), print_int(tuple.c));
}

static auto _print_no_weight_edge(Tuple const& tuple) -> std::string
{
    return std::format("{},{}\n", print_id(tuple.k), print_id(tuple.b));
}

auto
die::edge_list::print_as_csv(Tuple const& tuple)
    -> std::string
{
    switch (auto type = tuple.type)
    {
      case _Weight: return _print_weight_edge(tuple);
      case _No_weight: return _print_no_weight_edge(tuple);
      default:
        throw std::runtime_error(std::format("Unkown tuple type when printing as csv {}\n", type));
    }
}
