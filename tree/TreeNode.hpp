#pragma once

#include "tree/exit_scope.hpp"
#include "tree/options.hpp"
#include "tree/types.hpp"
#include "tree/Key.hpp"

#include <compare>
#include <concepts>
#include <mutex>
#include <shared_mutex>
#include <optional>
#include <utility>

namespace tree
{	
	template <class Value>
	struct TreeNode
	{
		Key const _key;
		std::optional<Value> _value{};
		std::unique_ptr<TreeNode> _child[2]{};
		std::shared_mutex mutable _lock{};
		
		constexpr TreeNode(Key key)
				: _key(key)
		{
		}

		template <class... Ts>
		requires (sizeof...(Ts) != 0) and std::constructible_from<Value, Ts...>
		constexpr TreeNode(Key key, Ts&&... ts)
				: _key(key)
				, _value(std::forward<Ts>(ts)...)
		{
		}

		constexpr TreeNode(std::unique_ptr<TreeNode> a, std::unique_ptr<TreeNode> b)
				: _key(a->_key ^ b->_key)
				, _child{ std::move(a), std::move(b) }
		{
			_canonicalize();
			_validate();
		}

		// The key is read only once it is constructed.
		constexpr auto key() const -> Key {
			return _key;
		}

		constexpr auto has_value() const -> bool {
			auto const _ = std::shared_lock(_lock);
			return _value.has_value();
		}

		constexpr auto value() const -> Value const& {
			auto const _ = std::shared_lock(_lock);
			return _value.value();
		}

		constexpr auto find(Key key, TreeNode const* best = nullptr) const
			-> TreeNode const*
		{
			assert(_key <= key);
			
			auto const _ = std::shared_lock(_lock);

			if (_value.has_value()) {
				best = this;
			}

			if (_child[0] and _child[0]->_key <= key) {
				return _child[0]->find(key);
			}

			if (_child[1] and _child[1]->_key <= key) {
				return _child[1]->find(key);
			}
			
			return best;
		}

		template <class... Ts>
		constexpr auto insert_or_update(Key key, Ts&&... ts) -> Value const&
		{
			auto const _ = exit_scope([&] {
				_validate();
			});
			
		  retry:
			{
				auto const _ = std::shared_lock(_lock);
			
				assert(_key <= key);

				if (key == _key) {
					if (_value.has_value()) {
						assert(_value.value() == Value(std::forward<Ts>(ts)...));
						return _value.value();
					}
					goto update;
				}
			
				if (_child[0] and _child[0]->_key <= key) {
					return _child[0]->insert_or_update(key, std::forward<Ts>(ts)...);
				}

				if (_child[1] and _child[1]->_key <= key) {
					return _child[1]->insert_or_update(key, std::forward<Ts>(ts)...);
				}
			}
			
			{
				auto const _ = std::unique_lock(_lock);
				
				if (_child[0] and _child[0]->_key <= key) {
					goto retry;
				}

				if (_child[1] and _child[1]->_key <= key) {
					goto retry;
				}
				
				return _insert(key, std::forward<Ts>(ts)...);
			}

		  update:
			{
				auto const _ = std::unique_lock(_lock);
				return _value.emplace(std::forward<Ts>(ts)...);
			}
		}
		
	  private:
		constexpr auto _validate() const -> void
		{
			if (_child[1]) {
				assert(_child[0]);
				assert(_key < _child[1]->_key);
			}

			if (_child[0]) {
				assert(_key < _child[0]->_key);
			}

			if (_child[0] and _child[1]) {
				assert(_child[0]->_key <=> _child[1]->_key == std::partial_ordering::unordered);
				assert(less(_child[0]->_key, _child[1]->_key));
			}
		}
		
		constexpr auto _canonicalize() -> void
		{
			if (not _child[1]) {
				return;
			}
			assert(_child[0]);
			if (not less(_child[0]->_key, _child[1]->_key)) {
				std::swap(_child[0], _child[1]);
			}
		}

		template <class... Ts>
		constexpr auto _insert(Key key, Ts&&... ts) -> Value const&
		{
			auto const _ = exit_scope([&] {
				_canonicalize();
				_validate();
			});
			
			auto node = std::make_unique<TreeNode>(key, std::forward<Ts>(ts)...);
			auto const& value = node->value();
			
			// if the node dominates either or both child, reorder them, I
			// hold the lock on this-> and node is not public yet
			if (_child[0] and key < _child[0]->_key) {
				node->_child[0] = std::move(_child[0]);
			}

			if (_child[1] and key < _child[1]->_key) {
				node->_child[1] = std::move(_child[1]);
			}

			node->_canonicalize();
			node->_validate();
			
			// case 1: _child[0] is null
			if (not _child[0]) {
				_child[0] = std::move(node);
				return value;
			}

			// case 2: _child[1] is null
			if (not _child[1]) {
				_child[1] = std::move(node);
				return value;
			}

			// case 3: I have two child
			auto const a = (_child[0]->_key ^ _child[1]->_key).size();
			auto const b = (_child[0]->_key ^ key).size();
			auto const c = (key ^ _child[1]->_key).size();
				
			// case 3a:
			if (a < b and b < c) {
				_child[1] = std::make_unique<TreeNode>(std::move(node), std::move(_child[1]));
				return value;
			}

			// case 3b:
			if (a < b) {
				_child[0] = std::make_unique<TreeNode>(std::move(node), std::move(_child[0]));
				return value;
			}

			// case 3c:
			if (a < c) {
				_child[1] = std::make_unique<TreeNode>(std::move(node), std::move(_child[1]));
				return value;
			}

			// case 3d:
			_child[0] = std::make_unique<TreeNode>(std::move(_child[0]), std::move(_child[1]));
			_child[1] = std::move(node);
			
			return value;
		}
	};
}

#ifdef TREE_TESTING

#include "tree/testing.hpp"

namespace tree::testing
{
	inline const auto test_insert = test<[] {
		TreeNode<int> root("0/0");
		assert(root._child[0] == nullptr);
		assert(root._child[1] == nullptr);
				
		{
			auto const& a = root.insert_or_update("1/128", 1);
			auto const* p = root.find("1/128", &root);
			assert(p == root._child[0].get());
			assert(root._child[0]->_child[0] == nullptr);
			assert(root._child[0]->_child[1] == nullptr);
			assert(root._child[1] == nullptr);

			assert(a == 1);
			assert(a == p->value());
			assert(&a == &p->value());
		}

		{
			auto const& a = root.insert_or_update("0/128", 42);
			auto const* p = root.find("0/128", &root);
			auto const* q = root.find("1/128", &root);
			assert(p == root._child[0].get());
			assert(q == root._child[1].get());
			
			assert(a == 42);
			assert(a == p->value());
			assert(&a == &p->value());
			assert(q->value() == 1);
		}

		return true;
	}>{};
}

#endif
