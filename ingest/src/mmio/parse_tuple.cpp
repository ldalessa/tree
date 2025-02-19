#include "ingest/mmio.hpp"
#include "ingest/utils.hpp"

auto
ingest::mmio::parse_tuple(char const* i, char const* e)
	-> std::optional<Tuple>
{
	return Tuple {
		.k = parse_id(i, e),
		.type = 0,
		.b = parse_id(skip(i, " "), e)
	};
}

auto
ingest::mmio::parse_tuple(std::string_view str)
    -> std::optional<Tuple>
{
    return parse_tuple(str.begin(), str.end());
}
