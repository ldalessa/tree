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

	inline void register_test(bool (*test)()) {
		tests.push_back(test);
	}

	template <bool (*op)(), bool check_constexpr = false>
	struct [[nodiscard]] test
	{
		test() {
			register_test(op);
		}		

		static constexpr auto check_result() -> bool {
			if constexpr (check_constexpr) {
				return std::invoke(op);
			}
			return true;
		}

#ifdef TREE_WITH_CONSTEXPR_CHECKS
		static_assert(check_result());
#endif
	};

	template <bool (*op)()>
	using cetest = test<op, true>;
}
