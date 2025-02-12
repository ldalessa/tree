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

		constexpr ~atomic_ptr() {
			delete _load();
		}

		constexpr atomic_ptr() noexcept = default;

		template <class U>
		constexpr atomic_ptr(U* ptr) noexcept : _ptr(ptr) {
		}

		constexpr atomic_ptr(atomic_ptr const&) = delete;
		constexpr auto operator=(atomic_ptr const&) = delete;

		template <class U>
		constexpr atomic_ptr(atomic_ptr<U>&& b) noexcept : _ptr(b._exchange(nullptr)) {
		}

		template <class U>
		constexpr auto operator=(atomic_ptr<U>&& b) noexcept -> atomic_ptr& {
			if (&b != this) {
				delete _exchange(b._exchange(nullptr));
			}
			return *this;
		}

		constexpr operator bool() const {
			return _load() != nullptr;
		}
		
		constexpr auto get() const -> T const* {
			return _load();
		}

		constexpr auto get() -> T* {
			return _load();
		}
		
		constexpr auto operator->() const -> T const* {
			return get();
		}

		constexpr auto operator->() -> T* {
			return get();
		}

		constexpr auto operator==(T const* t) const -> bool {
			return _load() == t;
		}

	private:
		constexpr auto _exchange(T* ptr) -> T* {
			if (std::is_constant_evaluated()) {
				return std::exchange(_ptr, ptr);
			}
			else {
				return std::atomic_ref(_ptr).exchange(ptr);
			}
		}
		
		constexpr auto _load() const -> T const* {
			return const_cast<atomic_ptr*>(this)->_load();
		}

		constexpr auto _load() -> T* {
			if (std::is_constant_evaluated()) {
				return _ptr;
			}
			else {
				return std::atomic_ref(_ptr).load();
			}
		}
	};

	template <class T, class... Args>
	requires std::constructible_from<T, Args&&...>
	inline constexpr auto make_atomic(Args&&... args) -> atomic_ptr<T> {
		return atomic_ptr<T>(new T(std::forward<Args>(args)...));
	}
}
