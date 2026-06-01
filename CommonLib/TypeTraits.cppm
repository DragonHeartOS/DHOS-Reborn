export module CommonLib:TypeTraits;
export {
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

	template<typename T>
	using RemoveReferenceT = typename RemoveReference<T>::Type;

	template<typename T> struct RemoveConst {
		using Type = T;
	};

	template<typename T> struct RemoveConst<T const> {
		using Type = T;
	};

	template<typename T> using RemoveConstT = typename RemoveConst<T>::Type;

	template<typename T>
	using RemoveConstRef = RemoveConstT<RemoveReferenceT<T>>;

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

	template<typename T>
	inline constexpr bool IsReferenceV = IsReference<T>::Value;

	template<typename T> struct IsIntegral {
		static constexpr bool Value = false;
	};

#define CL_DEFINE_INTEGRAL_TRAIT(T) \
	template<> struct IsIntegral<T> { \
		static constexpr bool Value = true; \
	};

	CL_DEFINE_INTEGRAL_TRAIT(bool)
	CL_DEFINE_INTEGRAL_TRAIT(char)
	CL_DEFINE_INTEGRAL_TRAIT(signed char)
	CL_DEFINE_INTEGRAL_TRAIT(unsigned char)
	CL_DEFINE_INTEGRAL_TRAIT(short)
	CL_DEFINE_INTEGRAL_TRAIT(unsigned short)
	CL_DEFINE_INTEGRAL_TRAIT(int)
	CL_DEFINE_INTEGRAL_TRAIT(unsigned int)
	CL_DEFINE_INTEGRAL_TRAIT(long)
	CL_DEFINE_INTEGRAL_TRAIT(unsigned long)
	CL_DEFINE_INTEGRAL_TRAIT(long long)
	CL_DEFINE_INTEGRAL_TRAIT(unsigned long long)
	CL_DEFINE_INTEGRAL_TRAIT(wchar_t)
	CL_DEFINE_INTEGRAL_TRAIT(char8_t)
	CL_DEFINE_INTEGRAL_TRAIT(char16_t)
	CL_DEFINE_INTEGRAL_TRAIT(char32_t)

#undef CL_DEFINE_INTEGRAL_TRAIT

	template<typename T>
	inline constexpr bool IsIntegralV = IsIntegral<T>::Value;

	template<typename T> T &&declval();

	template<typename Fn, typename... Args>
	using InvokeResult = decltype(declval<Fn>()(declval<Args>()...));

	template<typename Fn, typename... Args>
	using InvokeResultT = InvokeResult<Fn, Args...>;

	template<typename T, typename U>
	concept SameAs = __is_same(T, U);

	template<typename T, typename... Ts>
	concept OneOf = (SameAs<T, Ts> || ...);

	template<typename T> struct IsFloatingPoint {
		static constexpr bool Value = false;
	};

#define CL_DEFINE_FLOATING_POINT_TRAIT(T) \
	template<> struct IsFloatingPoint<T> { \
		static constexpr bool Value = true; \
	};

	CL_DEFINE_FLOATING_POINT_TRAIT(float)
	CL_DEFINE_FLOATING_POINT_TRAIT(double)
	CL_DEFINE_FLOATING_POINT_TRAIT(long double)

#undef CL_DEFINE_FLOATING_POINT_TRAIT

	template<typename T>
	inline constexpr bool IsFloatingPointV = IsFloatingPoint<T>::Value;

	template<typename T> struct IsEnum {
		static constexpr bool Value = __is_enum(T);
	};

	template<typename T> inline constexpr bool IsEnumV = IsEnum<T>::Value;

	template<typename T> struct IsUnsignedIntegral {
		static constexpr bool Value = IsIntegralV<T> && (T(-1) > T(0));
	};
	template<typename T>
	inline constexpr bool IsUnsignedIntegralV = IsUnsignedIntegral<T>::Value;

	template<typename T> struct IsSignedIntegral {
		static constexpr bool Value = IsIntegralV<T> && (T(-1) < T(0));
	};
	template<typename T>
	inline constexpr bool IsSignedIntegralV = IsSignedIntegral<T>::Value;

	}
}
