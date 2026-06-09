export module CommonLib:StringView;

import :StringOps;
import :Types;
import :Iterator;
import :Option;
import :Span;

export {
	namespace CL {

	/// @brief A non-owning view of a string of characters.
	/// @tparam CharTypeT The type of the characters in the string view.
	template<typename CharTypeT>
	struct BaseStringView
	    : detail::StringViewOps<BaseStringView<CharTypeT>, CharTypeT> {
		struct Iter : Iterator<Iter> {
			CharTypeT const *current { nullptr };
			CharTypeT const *end { nullptr };

			auto next() -> Option<CharTypeT const &>
			{
				if (current == end)
					return {};

				return *current++;
			}

			auto next_back() -> Option<CharTypeT const &>
			{
				if (current == end)
					return {};

				--end;
				return *end;
			}
		};

		static constexpr auto length(CharTypeT const *c_str) -> usize
		{
			usize len {};
			while (c_str[len] != '\0')
				len++;

			return len;
		}

		constexpr BaseStringView()
		    : BaseStringView { "" }
		{
		}

		constexpr BaseStringView(CharTypeT const *c_str)
		    : m_data { c_str }
		    , m_size { length(c_str) }
		{
		}

		constexpr BaseStringView(CharTypeT const *data, usize size)
		    : m_data { data }
		    , m_size { size }
		{
		}

		/// @brief Get a pointer to the characters in the string view.
		/// @return A pointer to the characters in the string view.
		constexpr auto data() -> CharTypeT const * { return m_data; }
		/// @brief Get a pointer to the characters in the string view.
		/// @return A pointer to the characters in the string view.
		constexpr auto data() const -> CharTypeT const * { return m_data; }

		/// @brief Get the size of the string view.
		/// @return The size of the string view.
		constexpr auto size() -> usize { return m_size; }
		/// @brief Get the size of the string view.
		/// @return The size of the string view.
		constexpr auto size() const -> usize { return m_size; }

		/// @brief Get an iterator over the characters in the string view.
		/// @return An iterator over the characters in the string view.
		constexpr auto iter() -> Iter
		{
			return Iter {
				.current = data(),
				.end = data() + size(),
			};
		}

		/// @brief Get an iterator over the characters in the string view.
		/// @return An iterator over the characters in the string view.
		constexpr auto iter() const -> Iter
		{
			return Iter {
				.current = data(),
				.end = data() + size(),
			};
		}

	private:
		CharTypeT const *m_data { nullptr };
		usize m_size {};
	};

	using StringView = BaseStringView<char>;

	}
}
