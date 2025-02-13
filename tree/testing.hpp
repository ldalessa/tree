#pragma once

#include <cassert>
#include <functional>
#include <vector>

namespace tree::testing
{
	std::vector<bool(*)()> tests;	

	inline void run_all_tests() {
		for (auto f : tests) {
			assert(std::invoke(f));
		}
	}

	inline void register_test(bool(*test)()) {
		tests.push_back(test);
	}

	template <bool (*op)()>
	struct [[nodiscard]] test
	{
		test() {
			register_test(op);
		}
	};
}
