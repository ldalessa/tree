#pragma once

#include "tree/exit_scope.hpp"
#include "tree/options.hpp"
#include "tree/types.hpp"
#include "tree/Key.hpp"

#include <compare>
#include <concepts>
#include <optional>
#include <utility>

namespace tree
{	
	template <class Value>
	struct NonBlockingTreeNode
	{
		using Node = NonBlockingTreeNode;
		
		struct value_box
		{
			Value* _data{};
			
			constexpr value_box() = default;

			constexpr value_box(Value* value)
				: _data(value)
			{
			}
			
			constexpr auto has_value() const -> bool {
				return _data != nullptr;
			}

			constexpr auto value() const -> Value const& {
				return *_data;
			}
		};
		
		struct alignas(16) pair
		{
			Node* _data[2]{};
			
			constexpr pair() = default;
			
			constexpr pair(Node* a, Node* b)
				: _data { a, b }
			{
				canonicalize();
			}

			constexpr auto operator[](std::size_t i) const -> Node* const& {
				return _data[i];
			}

			constexpr auto operator[](std::size_t i) -> Node*& {
				return _data[i];
			}

			constexpr auto canonicalize() -> pair&
			{
				using std::swap;

				if (not _data[0]) {
					if (_data[1]) {
						swap(_data[0], _data[1]);
					}
					return *this;
				}

				if (not _data[1]) {
					return *this;
				}
				
				if (less(_data[1]->_key, _data[0]->_key)) {
					swap(_data[0], _data[1]);
				}
				
				return *this;
			}

			constexpr auto validate(Key key) const -> void
			{
				if (_data[1]) {
					assert(_data[0]);
					assert(key < _data[1]->_key);
				}

				if (_data[0]) {
					assert(key < _data[0]->_key);
				}

				if (_data[0] and _data[1]) {
					assert(_data[0]->_key <=> _data[1]->_key == std::partial_ordering::unordered);
					assert(less(_data[0]->_key, _data[1]->_key));
				}
			}
		};
		
		Key const _key;
		std::atomic<value_box> _value{};
		std::atomic<pair> _child{};

		constexpr ~NonBlockingTreeNode() {
			delete _value.load()._data;
			delete _child.load()._data[0];
			delete _child.load()._data[1];
		}
		
		constexpr NonBlockingTreeNode(Key key)
				: _key(key)
		{
		}

		template <class... Ts>
		requires (sizeof...(Ts) != 0) and std::constructible_from<Value, Ts...>
		constexpr NonBlockingTreeNode(Key key, Ts&&... ts)
				: _key(key)
                , _value(new Value(std::forward<Ts>(ts)...))
		{
		}

		constexpr NonBlockingTreeNode(Node *a, Node *b)
				: _key(a->_key ^ b->_key)
				, _child(pair(a, b))
		{
		}

		// The key is read only once it is constructed.
		constexpr auto key() const -> Key {
			return _key;
		}

		constexpr auto has_value() const -> bool {
			return _value.load().has_value();
		}

		constexpr auto value() const -> Value const& {
			return _value.load().value();
		}

		constexpr auto name() const -> std::string {
			return std::format("{}", _key);
		}
		
		constexpr auto find(Key key, Node const* best = nullptr) const
			-> Node const*
		{
			assert(_key <= key);

			if (has_value()) {
				best = this;
			}

			auto const child = _child.load();
			
			if (child[0] and child[0]->_key <= key) {
				return child[0]->find(key, best);
			}

			if (child[1] and child[1]->_key <= key) {
				return child[1]->find(key, best);
			}
			
			return best;
		}

		template <class... Ts>
		constexpr auto insert_or_update(Key key, Ts&&... ts) -> Value const&
		{
			assert(_key <= key);

			if (key == _key) {
				return _update(std::forward<Ts>(ts)...);
			}

			auto const child = _child.load();
			
			if (child[0] and child[0]->_key <= key) {
				return child[0]->insert_or_update(key, std::forward<Ts>(ts)...);
			}
			
			if (child[1] and child[1]->_key <= key) {
				return child[1]->insert_or_update(key, std::forward<Ts>(ts)...);
			}
	
			return _insert(child, key, std::forward<Ts>(ts)...);
		}
		
	  private:
		template <class... Ts>
		constexpr auto _update(Ts&&... ts) -> Value const&
		{
			auto value = new Value(std::forward<Ts>(ts)...);
			auto old = _value.exchange(value_box(value));
			delete old._data;
			return *value;
		}
		
		template <class... Ts>
		constexpr auto _insert(pair child, Key key, Ts&&... ts) -> Value const&
		{
			auto expected = child;
			
			auto const cas_child = [&](pair child, Node* node, auto... nodes) -> Value const&
			{
				child.canonicalize();
				child.validate(_key);
				if (not _child.compare_exchange_strong(expected, child)) {
					((node->_child = pair{}, delete node), ..., (nodes->_child = pair{}, delete nodes));
					return insert_or_update(key, std::forward<Ts>(ts)...);
				}
				return node->value();
			};
			
			auto node = new Node(key, std::forward<Ts>(ts)...);
			
			{
				pair p{};
				
				// if the node dominates either or both child, reorder them
				if (child[0] and key < child[0]->_key) {
					p[0] = std::exchange(child[0], nullptr);
				}

				if (child[1] and key < child[1]->_key) {
					p[1] = std::exchange(child[1], nullptr);
				}

				p.canonicalize();
				p.validate(key);
				node->_child.store(p);
			}

			// case 1: _child[0] is null
			if (not child[0]) {
				child[0] = node;
				return cas_child(child, node);
			}

			// case 2: _child[1] is null
			if (not child[1]) {
				child[1] = node;
				return cas_child(child, node);
			}

			// case 3: I have two child
			auto const a = (child[0]->_key ^ child[1]->_key).size();
			auto const b = (child[0]->_key ^ key).size();
			auto const c = (key ^ child[1]->_key).size();
				
			// case 3a:
			if (a < b and b < c) {
				child[1] = new Node(node, std::exchange(child[1], nullptr));
				return cas_child(child, node, child[1]);
			}

			// case 3b:
			if (a < b) {
				child[0] = new Node(node, std::exchange(child[0], nullptr));
				return cas_child(child, node, child[0]);
			}

			// case 3c:
			if (a < c) {
				child[1] = new Node(node, std::exchange(child[1], nullptr));
				return cas_child(child, node, child[1]);
			}

			// case 3d:
			child[0] = new Node(std::exchange(child[0], nullptr), std::exchange(child[1], nullptr));
			child[1] = node;
			return cas_child(child, node, child[0]);
		}
	};
}

#ifdef TREE_TESTING

#include "tree/testing.hpp"

namespace tree::testing
{
	inline const auto test_insert = test<[] {
		NonBlockingTreeNode<int> root("0/0");
		assert(root._child.load()[0] == nullptr);
		assert(root._child.load()[1] == nullptr);
				
		{
			auto const& a = root.insert_or_update("1/128", 1);
			auto const* p = root.find("1/128", &root);
			assert(p == root._child.load()[0]);
			assert(root._child.load()[0]->_child.load()[0] == nullptr);
			assert(root._child.load()[0]->_child.load()[1] == nullptr);
			assert(root._child.load()[1] == nullptr);

			assert(a == 1);
			assert(a == p->value());
			assert(&a == &p->value());
		}

		{
			auto const& a = root.insert_or_update("0/128", 42);
			auto const* p = root.find("0/128", &root);
			auto const* q = root.find("1/128", &root);
			assert(p == root._child.load()[0]);
			assert(q == root._child.load()[1]);
			
			assert(a == 42);
			assert(a == p->value());
			assert(&a == &p->value());
			assert(q->value() == 1);
		}

		return true;
	}>{};
}

#endif
