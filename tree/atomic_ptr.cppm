export module tree:atomic_ptr;

import std;

export
namespace tree
{
	template <class T>
	struct atomic_ptr
	{
		template <class> friend struct atomic_ptr;
		
		T* _ptr{};

		constexpr atomic_ptr() noexcept = default;

		template <class U>
		constexpr atomic_ptr(U* ptr) noexcept : _ptr(ptr) {
		}

		template <class U>
		constexpr atomic_ptr(atomic_ptr<U> const& b) : _ptr(b.get()) {
		}

		template <class U>
		constexpr auto operator=(atomic_ptr<U> const& b) -> atomic_ptr& {
			return set(b.get());
		}
		
		constexpr operator bool() const {
			return get() != nullptr;
		}

		constexpr auto get() const -> T const* {
			return const_cast<atomic_ptr*>(this)->get();
		}

		constexpr auto get() -> T* {
			if (std::is_constant_evaluated()) {
				return _ptr;
			}
			else {
				return std::atomic_ref(_ptr).load();
			}
		}

		template <class U>
		constexpr auto set(U* t) -> atomic_ptr& {
			std::atomic_ref(_ptr).store(t);
			return *this;
		}
		
		constexpr auto operator->() const -> T const* {
			return get();
		}

		constexpr auto operator->() -> T* {
			return get();
		}

		constexpr auto operator==(T const* t) const -> bool {
			return get() == t;
		}

		constexpr auto cas(T* from, T* to) -> bool {
			return false;
		}		

		constexpr auto exchange(T* ptr) -> T* {
			if (std::is_constant_evaluated()) {
				return std::exchange(_ptr, ptr);
			}
			else {
				return std::atomic_ref(_ptr).exchange(ptr);
			}
		}
	};

	template <class T, class... Args>
	requires std::constructible_from<T, Args&&...>
	inline constexpr auto make_atomic(Args&&... args) -> atomic_ptr<T> {
		return atomic_ptr<T>(new T(std::forward<Args>(args)...));
	}
}
