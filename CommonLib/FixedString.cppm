export module CommonLib:FixedString;

import :Array;
import :String;
import :StringView;
import :Types;

export {
	namespace CL {

	/// @brief Like a CL::String but with a fixed size backing storage.
	/// @tparam CharTypeT The type of the characters in the string.
	/// @tparam CapacityT The capacity of the backing storage.
	template<typename CharTypeT, usize CapacityT> struct BaseFixedString {
		constexpr BaseFixedString() { m_data[0] = '\0'; }

		constexpr BaseFixedString(CharTypeT const *cstring)
		{
			usize len {};
			while (len < CapacityT && cstring[len] != '\0') {
				m_data[len] = cstring[len];
				len++;
			}

			m_size = len;
			m_data[m_size] = '\0';
		}

		constexpr explicit BaseFixedString(
		    BaseStringView<CharTypeT> const &string)
		{
			usize len = string.size();
			if (len > CapacityT)
				len = CapacityT;

			for (usize i {}; i < len; ++i)
				m_data[i] = string.data()[i];

			m_size = len;
			m_data[m_size] = '\0';
		}

		static constexpr auto capacity() -> usize { return CapacityT; }

		constexpr auto size() const -> usize { return m_size; }

		constexpr auto is_empty() const -> bool { return m_size == 0; }

		constexpr auto data() -> CharTypeT * { return &m_data[0]; }
		constexpr auto data() const -> CharTypeT const * { return &m_data[0]; }

		constexpr auto c_str() const -> CharTypeT const * { return &m_data[0]; }

		constexpr auto view() const -> BaseStringView<CharTypeT>
		{
			return { data(), size() };
		}

		auto clear() -> void
		{
			m_size = 0;
			m_data[0] = '\0';
		}

		auto append(CharTypeT ch) -> void
		{
			if (m_size >= CapacityT)
				return;

			m_data[m_size] = ch;
			m_size++;
			m_data[m_size] = '\0';
		}

		auto append(BaseStringView<CharTypeT> const &string) -> void
		{
			usize available = CapacityT - m_size;
			usize to_copy = string.size();
			if (to_copy > available)
				to_copy = available;

			for (usize i {}; i < to_copy; ++i)
				m_data[m_size + i] = string.data()[i];

			m_size += to_copy;
			m_data[m_size] = '\0';
		}

		auto operator+=(CharTypeT ch) -> BaseFixedString &
		{
			append(ch);
			return *this;
		}

		auto operator+=(BaseStringView<CharTypeT> const &string)
		    -> BaseFixedString &
		{
			append(string);
			return *this;
		}

		constexpr auto operator[](usize index) -> CharTypeT &
		{
			return m_data[index];
		}

		constexpr auto operator[](usize index) const -> CharTypeT const &
		{
			return m_data[index];
		}

		constexpr auto starts_with(
		    BaseStringView<CharTypeT> const &prefix) const -> bool
		{
			if (prefix.size() > size())
				return false;

			for (usize i {}; i < prefix.size(); ++i) {
				if (data()[i] != prefix.data()[i])
					return false;
			}

			return true;
		}

		constexpr auto ends_with(BaseStringView<CharTypeT> const &suffix) const
		    -> bool
		{
			if (suffix.size() > size())
				return false;

			usize offset = size() - suffix.size();

			for (usize i {}; i < suffix.size(); ++i) {
				if (data()[offset + i] != suffix.data()[i])
					return false;
			}

			return true;
		}

		constexpr auto find(BaseStringView<CharTypeT> const &needle) const
		    -> usize
		{
			if (needle.size() == 0)
				return 0;

			if (needle.size() > size())
				return npos;

			for (usize i {}; i <= size() - needle.size(); ++i) {
				bool found = true;

				for (usize j {}; j < needle.size(); ++j) {
					if (data()[i + j] != needle.data()[j]) {
						found = false;
						break;
					}
				}

				if (found)
					return i;
			}

			return npos;
		}

		constexpr auto contains(BaseStringView<CharTypeT> const &needle) const
		    -> bool
		{
			return find(needle) != npos;
		}

		auto substring(usize start, usize count) const
		    -> BaseStringView<CharTypeT>
		{
			if (start >= size())
				return {};

			if (start + count > size())
				count = size() - start;

			return BaseStringView(data() + start, count);
		}

		constexpr auto operator==(BaseStringView<CharTypeT> const &other) const
		    -> bool
		{
			return view() == other;
		}

		constexpr auto operator==(BaseFixedString const &other) const -> bool
		{
			return view() == other.view();
		}

		constexpr auto operator!=(BaseStringView<CharTypeT> const &other) const
		    -> bool
		{
			return !(*this == other);
		}

		constexpr auto operator!=(BaseFixedString const &other) const -> bool
		{
			return !(*this == other);
		}

		static constexpr usize npos = static_cast<usize>(-1);

	private:
		Array<CharTypeT, CapacityT + 1> m_data {};
		usize m_size {};
	};

	template<usize CapacityT>
	using FixedString = BaseFixedString<char, CapacityT>;

	namespace detail::adl {

	template<usize N>
	inline auto to_display_string(BaseFixedString<char, N> const &value)
	    -> String
	{
		return String(value.view());
	}

	template<usize N>
	inline auto to_debug_string(BaseFixedString<char, N> const &value) -> String
	{
		return String(value.view());
	}

	template<usize N>
	inline auto to_hash(BaseFixedString<char, N> const &value) -> usize
	{
		return hash_string(value.view());
	}

	}

	}
}
