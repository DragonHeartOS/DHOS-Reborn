export module CommonLib:Pair;

export {
	namespace CL {

	template<typename FirstT, typename SecondT> struct Pair {
		FirstT first;
		SecondT second;

		constexpr bool operator==(Pair const &) const = default;
	};

	}
}
