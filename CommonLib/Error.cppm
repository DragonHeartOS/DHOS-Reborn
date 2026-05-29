export module CommonLib:Error;

import :Result;
import :String;
import :TypeTraits;
import :Utility;
import :Variant;

export {
	namespace CL {

	template<typename... Es> using Error = Variant<Es...>;

	struct ErasedError;

	namespace detail {

	template<typename...> using VoidT = void;

	template<typename T> struct AlwaysFalse {
		static constexpr bool Value = false;
	};

	template<typename T> struct IsErasedError {
		static constexpr bool Value = false;
	};

	template<typename... Es> struct IsVariantError {
		static constexpr bool Value = false;
	};

	template<typename... Es> struct IsVariantError<Variant<Es...>> {
		static constexpr bool Value = true;
	};

	template<typename T> struct IsErrorCompatible {
		using Type = RemoveConstRef<T>;
		static constexpr bool Value
		    = IsErasedError<Type>::Value || IsVariantError<Type>::Value;
	};

	template<typename T, typename = void> struct HasDisplayString {
		static constexpr bool Value = false;
	};

	template<typename T>
	struct HasDisplayString<T,
	    VoidT<decltype(String(to_display_string(declval<T>())))>> {
		static constexpr bool Value = true;
	};

	template<typename T>
	inline constexpr bool HasDisplayStringV = HasDisplayString<T>::Value;

	template<typename T> auto erase_error_value(T &&value) -> ErasedError;
	template<typename VariantType, int I = 0>
	auto erase_error_variant(VariantType const &error) -> ErasedError;

	}

	struct ErasedError {
		ErasedError() = default;
		ErasedError(String message)
		    : m_message(move(message))
		{
		}

		ErasedError(StringView const message)
		    : m_message(message)
		{
		}

		ErasedError(char const *message)
		    : m_message(message)
		{
		}

		static auto msg(StringView const message) -> ErasedError
		{
			return ErasedError(message);
		}

		template<typename E> static auto from(E &&error) -> ErasedError
		{
			using ErrorType = RemoveConstRef<E>;
			static_assert(detail::IsErrorCompatible<ErrorType>::Value,
			    "ErasedError::from(E): E must be CL::ErasedError or "
			    "CL::Error<...>");

			if constexpr (detail::IsErasedError<ErrorType>::Value) {
				return forward<E>(error);
			} else {
				return detail::erase_error_variant(error);
			}
		}

		constexpr auto message() const -> StringView
		{
			return m_message.view();
		}

	private:
		String m_message;
	};

	namespace ErrorsV {
	struct HashMapDuplicateKeyError { };
	}

	namespace detail::adl {

	inline auto to_display_string(ErrorsV::HashMapDuplicateKeyError const &)
	    -> String
	{
		return String("HashMap duplicate key");
	}

	}

	using Errors = Error<ErrorsV::HashMapDuplicateKeyError>;

	namespace detail {

	template<> struct IsErasedError<ErasedError> {
		static constexpr bool Value = true;
	};

	template<typename T> auto erase_error_value(T &&value) -> ErasedError
	{
		using ValueType = RemoveConstRef<T>;
		static_assert(HasDisplayStringV<ValueType const &>,
		    "ErasedError::from(E): E must support to_display_string(E) "
		    "returning "
		    "CL::String-compatible type");

		return ErasedError(String(to_display_string(forward<T>(value))));
	}

	template<typename... Es, int I>
	auto erase_error_variant(Variant<Es...> const &error) -> ErasedError
	{
		if constexpr (I >= sizeof...(Es)) {
			__builtin_unreachable();
		} else {
			auto value = error.template get<I>();
			if (value.is_some())
				return erase_error_value(*value);

			return erase_error_variant<Variant<Es...>, I + 1>(error);
		}
	}

	}

	template<typename T> using Fallible = Result<T, ErasedError>;

	}
}
