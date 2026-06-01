export module CommonLib:TypeList;

import :Types;

export {
	namespace CL {

	template<typename... Ts> struct TypeList { };

	template<typename T> struct IsTypeList {
		static constexpr bool Value = false;
	};

	template<typename... Ts> struct IsTypeList<TypeList<Ts...>> {
		static constexpr bool Value = true;
	};

	template<typename T>
	inline constexpr bool IsTypeListV = IsTypeList<T>::Value;

	template<typename List> struct TypeListSize;

	template<typename... Ts> struct TypeListSize<TypeList<Ts...>> {
		static constexpr usize Value = sizeof...(Ts);
	};

	template<typename List>
	inline constexpr usize TypeListSizeV = TypeListSize<List>::Value;

	template<usize Index, typename List> struct TypeListAt;

	template<usize Index> struct TypeListAt<Index, TypeList<>> {
		static_assert(Index != Index, "TypeListAt index out of range");
	};

	template<usize Index, typename Head, typename... Tail>
	struct TypeListAt<Index, TypeList<Head, Tail...>> {
		using Type = typename TypeListAt<Index - 1, TypeList<Tail...>>::Type;
	};

	template<typename Head, typename... Tail>
	struct TypeListAt<0, TypeList<Head, Tail...>> {
		using Type = Head;
	};

	template<usize Index, typename List>
	using TypeListAtT = typename TypeListAt<Index, List>::Type;

	template<usize... I> struct IndexSequence { };

	template<usize N, usize... I>
	struct MakeIndexSequenceImpl
	    : MakeIndexSequenceImpl<N - 1, N - 1, I...> { };

	template<usize... I> struct MakeIndexSequenceImpl<0, I...> {
		using Type = IndexSequence<I...>;
	};

	template<usize N>
	using MakeIndexSequence = typename MakeIndexSequenceImpl<N>::Type;

	}
}
