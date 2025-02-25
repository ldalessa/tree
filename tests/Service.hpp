#pragma once

#include "tree/types.hpp"
#include "tree/GlobTreeNode.hpp"
#include "tree/TopLevelTree.hpp"

#include <expected>
#include <utility>
#include <vector>

namespace tree::tests
{
	struct Service
	{
		TopLevelTree& _tlt;
		GlobTreeNode<u128> _globs{"0/0"};
		u32 _id;
		
		constexpr Service(u32 id, u32 n_services, TopLevelTree& tlt)
				: _tlt(tlt)
				, _id(id)
		{
		}

		constexpr auto insert(u128 key) -> std::expected<bool, std::vector<u128>>
		{
			if (auto insert = _globs.insert(key)) {
				return std::move(insert).value();
			}
			else {
				GlobTreeNode glob = std::move(insert).error();
				if (_tlt.owner(glob.key()) == _id) {
					_globs.reinsert(std::move(glob));
					return true;
				}
				_tlt.insert(glob.key());
				return std::unexpected(glob.take_value().take_all());
			}
		}

		constexpr auto contains(u128 key) -> bool {
			return _globs.find(key, nullptr);
		}

		constexpr auto for_each_node(auto&& f) const -> void {
			_globs.for_each_node(f);
		}
	};
}
