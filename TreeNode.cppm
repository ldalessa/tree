module;

#include <cassert>

export module tree:TreeNode;

import :exit_scope;
import :types;
import :Key;
import std;

export
namespace tree
{
	template <class> struct SyntheticNode{};
	template <class Value> struct ConcreteNode {
		Value value;
	};
  
	template <class Value>
	struct TreeNode : private std::variant<SyntheticNode<Value>, ConcreteNode<Value>>
	{
		using cnode = ConcreteNode<Value>;
		using snode = SyntheticNode<Value>;
    
		Key _key;
		TreeNode* _parent{};
		std::unique_ptr<TreeNode> _children[2]{};

		TreeNode(Key key)
			: TreeNode::variant::variant(snode())
			, _key(key)
		{
		}

		TreeNode(Key key, Value value)
			: TreeNode::variant::variant(cnode(std::move(value)))
			, _key(key)
		{
		}

		TreeNode(std::unique_ptr<TreeNode> a, std::unique_ptr<TreeNode> b)
			: _key(a->_key ^ b->_key)
			, _children{std::move(a), std::move(b)}
		{
			assert(_children[0] and _children[1]);
			_children[0]->_parent = this;
			_children[1]->_parent = this;
			_canonicalize();
			_validate();
		}	
		
		constexpr auto key() const -> Key {
			return _key;
		}
		
		constexpr auto name() const -> std::string {
			return std::format("{}", _key);
		}

		struct find_result {
			TreeNode* node;
			TreeNode* best;
		};

		constexpr auto find(Key key, TreeNode* best) -> find_result
		{
			assert(_key <= key or best != nullptr);

			if (std::holds_alternative<cnode>(*this)) {
				best = this;
			}

			if (_children[1] < key) {
				assert(_children[0] and key < _children[0]);
				return _children[1]->find(key, best);
			}

			if (_children[0] < key) {
				return _children[1]->find(key, best);
			}
            
			return find_result {
				.node = best,
				.best = this
			};
		}

		constexpr auto insert(Key key, Value value) -> TreeNode&
		{
			assert(_key <= key and not (_children[0] <= key) and not (_children[1] <= key));
			
			if (_key == key) {
				return set_value(std::move(value));
			}

			auto const _ = exit_scope([&] {
				_canonicalize();
				_validate();
			});
			
			auto node = std::make_unique<TreeNode>(key, std::move(value));
			
			if (key < _children[0]) {
				node->_set_child(0, _take_child(0));
			}

			if (key < _children[1]) {
				node->_set_child(1, _take_child(1));
			}

			if (_children[0] == nullptr) {
				return *_set_child(0, std::move(node));
			}

			if (_children[1] == nullptr) {
				return *_set_child(1, std::move(node));
			}

			_set_child(0, std::make_unique<TreeNode>(_take_child(0), _take_child(1)));
			_set_child(1, std::move(node));
			return *_children[1];
		}

		constexpr auto set_value(Value value) -> TreeNode&
		{
			if (auto p = std::get_if<cnode>(this)) {
				p->value = std::move(value);
				return *this;
			}
			
			return *this = TreeNode(_key, std::move(value));
		}
		
	private:
		constexpr auto _validate() const -> void
		{
			if (_parent) {
				assert(_parent->_key < _key);
				assert(_parent->_children[0].get() == this or _parent->_children[1].get() == this);
			}

			if (_children[1]) {
				assert(_children[0]);
				assert(_key < _children[1]->_key);
				assert(_children[1]->_parent == this);
			}

			if (_children[0]) {
				assert(_key < _children[0]->_key);
				assert(_children[0]->_parent == this);
			}

			if (_children[0] and _children[1]) {
				assert(_children[0]->_key <=> _children[1]->_key == std::partial_ordering::unordered);
			}
		}
		
		constexpr auto _canonicalize() -> void
		{
			if (_children[1] == nullptr) {
				return;
			}

			if (_children[0] == nullptr) {
				_children[0] = std::move(_children[1]);
			}

			auto const c = _children[0]->_key ^ _children[1]->_key;
			auto const b = c.size();
			assert(b < 128u);
			
			if (_children[0]->_key[b] == (u64)0) {
				assert(_children[1]->_key[b] == (u64)1);
				return;
			}

			std::swap(_children[0], _children[1]);
		}
		
		constexpr auto _set_child(int n, std::unique_ptr<TreeNode> c) -> TreeNode*
		{
			assert(0 <= n and n < 2);
			if ((_children[n] = std::move(c))) {
				_children[n]->_parent = this;
			}
			return _children[n].get();
		}
		
		constexpr auto _take_child(int n) -> std::unique_ptr<TreeNode>
		{
			assert(0 <= n and n < 2);
			if (_children[n]) {
				_children[n]->_parent = nullptr;
			}
			return std::move(_children[n]);
		}

		constexpr friend auto operator<(Key a, std::unique_ptr<TreeNode> const& b)
			-> ARROW(not b or a < b->_key);

		constexpr friend auto operator<(std::unique_ptr<TreeNode> const& a, Key b)
			-> ARROW(a and a->_key < b);

		constexpr friend auto operator<=(Key a, std::unique_ptr<TreeNode> const& b)
			-> ARROW(not b or a <= b->_key);

		constexpr friend auto operator<=(std::unique_ptr<TreeNode> const& a, Key b)
			-> ARROW(a and a->_key <= b);
	};
}
