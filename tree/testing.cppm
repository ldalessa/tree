module;

#include <cassert>

export module tree:testing;

import std;

namespace tree::testing
{
	export
	std::vector<bool(*)()> tests;
	
	export
	inline void run_all_tests() {
		for (auto f : tests) {
			assert(std::invoke(f));
		}
	}

	void register_test(bool(*test)()) {
		tests.push_back(test);
	}

	template <bool (*op)()>
	struct [[nodiscard]] test
	{
		test() {
			register_test(op);
		}

#ifdef TREE_WITH_CONSTEXPR_CHECKS
		static_assert(std::invoke(op));
#endif
	};
}
