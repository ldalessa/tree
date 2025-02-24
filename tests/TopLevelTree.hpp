#pragma once

#include "tree/types.hpp"
#include "tree/Key.hpp"
#include "tree/NonBlockingTreeNode.hpp"
#include <bit>
#include <optional>

namespace tree::tests
{
	struct TopLevelTree
	{
		NonBlockingTreeNode<u32> _tree;
		u32 _n_services;
		u32 _n_service_bits;
		
		constexpr TopLevelTree(u32 n_services, bool construct_roots = true)
			: _tree("0/0")
			, _n_services(n_services)
			, _n_service_bits(std::countr_zero(std::bit_ceil(n_services)))
		{
			if (not construct_roots) {
				return;
			}
			
			for (u32 i = 0; i < n_services; ++i) {
				auto const j = u128(i);
				auto const bits = std::rotr(j, _n_service_bits);
				auto const key = Key(bits, _n_service_bits);
				auto const service = _close_mapping(key);
				assert(service == _get_source_service(key.source()));
				_tree.insert_or_update(key, service);
			}
		}
		
		constexpr auto try_lookup(u128 key) -> std::optional<u32> {
			if (auto node = _tree.find(key, nullptr)) {
				return node->value();
			}
			return std::nullopt;
		}
		
		constexpr auto lookup(u128 key) -> u32 {
			return _tree.find(key)->value();
		}
		
		constexpr auto insert(Key const& key) -> u32 {
			auto const service = _close_mapping(key);
			return _tree.insert_or_update(key, service);
		}

		constexpr auto owner(Key const& key) -> u32 {
			return _close_mapping(key);
		}
		
	private:
		static constexpr auto _bitswap(u64 x) -> u64
		{   
			// swap odd and even bits
			x = ((x >> 1) & 0x5555555555555555) | ((x & 0x5555555555555555) << 1);
			// swap consecutive pairs
			x = ((x >> 2) & 0x3333333333333333) | ((x & 0x3333333333333333) << 2);
			// swap nibbles ... 
			x = ((x >> 4) & 0x0F0F0F0F0F0F0F0F) | ((x & 0x0F0F0F0F0F0F0F0F) << 4);
			// reverse bytes
			return std::byteswap(x);
		}

		static constexpr auto _shift_left(u64 x, u32 n) -> u64 {
			return (n == 64) ? 0 : x << n;
		}
		
		constexpr auto _close_mapping(Key const& key) -> u32
		{
			// abcd .... | ABCD ....
			// s = 000 ... abcd
			// t = 000 ... DCBA
			//  mask = 111 ... 100
			// ~mask = 000 ... 011
			// level           ^
			//
			// Take level least significant bits (~mask) from the reversed
			// target and the rest (mask) from the source.
			
			u32 const s = _get_source_service(key.source());
			u32 const t = _get_reversed_target_service(key.target());

			u32 const level = std::sub_sat(key.size(), 64u);
			u32 const mask = -1u << level;

			u32 const service = (mask & s) | (~mask & t);
			assert(service < _n_services);
			return service;
		}

		constexpr auto _mask_service(u64 x) const -> u64 {
			u64 const mask = _shift_left(-1_u64, 64 - _n_service_bits);
			return x & mask;
		}

		constexpr auto _get_source_service(u64 source) const -> u64 {
			return std::rotl(_mask_service(source), _n_service_bits);
		}

		constexpr auto _get_reversed_target_service(u64 target) const -> u64 {
			return _bitswap(_mask_service(target));
		}		
	};

}
