#include "ingest/mmio.hpp"
#include "ingest/utils.hpp"
#include <format>

auto
ingest::mmio::print_tuple(Tuple const& t)
    -> std::string
{
    return std::format("{} {}", print_id(t.k), print_id(t.b));
}

