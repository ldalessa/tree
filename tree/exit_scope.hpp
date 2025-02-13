#pragma once

namespace tree
{
	template <class Op>
	struct exit_scope {
		Op op;
		constexpr ~exit_scope() {
			op();
		}
	};
}
