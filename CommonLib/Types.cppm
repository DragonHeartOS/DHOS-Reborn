module;
#include <stdint.h>

export module CommonLib:Types;
export {
	using uint = unsigned int;

	using u8 = unsigned char;
	using u16 = unsigned short;
	using u32 = unsigned int;
	using u64 = unsigned long;

	using i8 = signed char;
	using i16 = short;
	using i32 = int;
	using i64 = long;

	using f32 = float;
	using f64 = double;

	using uptr = unsigned long;
	using usize = decltype(sizeof(0));
}
