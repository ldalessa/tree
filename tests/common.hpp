#pragma once

#include "tree/types.hpp"
#include "ingest/mmio.hpp"

namespace tree::tests {
	inline constexpr auto tuple_to_key(ingest::Tuple const& tuple) -> u128 {
		return ((u128)(tuple.k) << 64) | tuple.b;
	}
}
