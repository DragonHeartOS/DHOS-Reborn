#pragma once

namespace CL {

template<typename T> struct RemoveReference {
	using Type = T;
};

template<typename T> struct RemoveReference<T &> {
	using Type = T;
};

template<typename T> struct RemoveReference<T &&> {
	using Type = T;
};

template<typename T> using RemoveReferenceT = typename RemoveReference<T>::Type;

template<typename T> struct RemoveConst {
	using Type = T;
};

template<typename T> struct RemoveConst<T const> {
	using Type = T;
};

template<typename T> using RemoveConstT = typename RemoveConst<T>::Type;

template<typename T> using RemoveConstRef = RemoveConstT<RemoveReferenceT<T>>;

template<typename T> struct AddConst {
	using Type = T const;
};

template<typename T> using AddConstT = typename AddConst<T>::Type;

template<typename T> struct IsReference {
	static constexpr bool Value = false;
};

template<typename T> struct IsReference<T &> {
	static constexpr bool Value = true;
};

template<typename T> struct IsReference<T &&> {
	static constexpr bool Value = true;
};

template<typename T> inline constexpr bool IsReferenceV = IsReference<T>::Value;

template<typename T> T &&declval();

template<typename Fn, typename... Args>
using InvokeResult = decltype(declval<Fn>()(declval<Args>()...));

template<typename Fn, typename... Args>
using InvokeResultT = InvokeResult<Fn, Args...>;

}
