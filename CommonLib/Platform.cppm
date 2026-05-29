export module CommonLib:Platform;

import :StringView;

export {
	namespace CL {

	[[noreturn]] void panic(StringView const message);

	}

	namespace std {
	struct nothrow_t {
		explicit nothrow_t() = default;
	};

	struct align_val_t {
		explicit align_val_t(usize a)
		    : value { a }
		{
		}

		usize value;
	};

	inline constexpr nothrow_t nothrow {};

	class bad_alloc { };
	}
	inline void *operator new(usize, void *ptr) noexcept { return ptr; }
	inline void *operator new[](usize, void *ptr) noexcept { return ptr; }
	inline void operator delete(void *, void *) noexcept { }
	inline void operator delete[](void *, void *) noexcept { }
}
