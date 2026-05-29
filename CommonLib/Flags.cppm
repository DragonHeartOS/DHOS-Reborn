export module CommonLib:Flags;

import :InitializerList;

export {
	namespace CL {

	template<typename Enum> struct Flags {
		static_assert(__is_enum(Enum), "Flags requires an enum type");

		using Underlying = __underlying_type(Enum);

		constexpr Flags() = default;

		constexpr Flags(Enum bit)
		    : m_bits { static_cast<Underlying>(bit) }
		{
		}

		constexpr explicit Flags(Underlying bits)
		    : m_bits { bits }
		{
		}

		constexpr Flags(InitializerList<Enum> bits)
		{
			for (auto bit : bits)
				m_bits |= static_cast<Underlying>(bit);
		}

		constexpr auto raw() const -> Underlying { return m_bits; }
		constexpr explicit operator Underlying() const { return m_bits; }

		constexpr auto any() const -> bool { return m_bits != 0; }
		constexpr auto none() const -> bool { return m_bits == 0; }

		constexpr auto contains(Enum bit) const -> bool
		{
			return (m_bits & static_cast<Underlying>(bit))
			    == static_cast<Underlying>(bit);
		}

		constexpr auto contains_any(Flags other) const -> bool
		{
			return (m_bits & other.m_bits) != 0;
		}

		constexpr auto contains_all(Flags other) const -> bool
		{
			return (m_bits & other.m_bits) == other.m_bits;
		}

		constexpr auto operator~() const -> Flags { return Flags(~m_bits); }

		constexpr auto operator|=(Flags rhs) -> Flags &
		{
			m_bits |= rhs.m_bits;
			return *this;
		}

		constexpr auto operator&=(Flags rhs) -> Flags &
		{
			m_bits &= rhs.m_bits;
			return *this;
		}

		constexpr auto operator^=(Flags rhs) -> Flags &
		{
			m_bits ^= rhs.m_bits;
			return *this;
		}

		constexpr auto operator+=(Flags rhs) -> Flags &
		{
			m_bits |= rhs.m_bits;
			return *this;
		}

		constexpr auto operator-=(Flags rhs) -> Flags &
		{
			m_bits &= static_cast<Underlying>(~rhs.m_bits);
			return *this;
		}

		friend constexpr auto operator==(Flags lhs, Flags rhs) -> bool
		    = default;

		friend constexpr auto operator|(Flags lhs, Flags rhs) -> Flags
		{
			return Flags(lhs.m_bits | rhs.m_bits);
		}

		friend constexpr auto operator&(Flags lhs, Flags rhs) -> Flags
		{
			return Flags(lhs.m_bits & rhs.m_bits);
		}

		friend constexpr auto operator^(Flags lhs, Flags rhs) -> Flags
		{
			return Flags(lhs.m_bits ^ rhs.m_bits);
		}

		friend constexpr auto operator+(Flags lhs, Flags rhs) -> Flags
		{
			return Flags(lhs.m_bits | rhs.m_bits);
		}

		friend constexpr auto operator-(Flags lhs, Flags rhs) -> Flags
		{
			return Flags(lhs.m_bits & static_cast<Underlying>(~rhs.m_bits));
		}

	private:
		Underlying m_bits {};
	};

	template<typename Enum>
	constexpr auto operator|(Flags<Enum> lhs, Enum rhs) -> Flags<Enum>
	{
		return lhs | Flags<Enum>(rhs);
	}

	template<typename Enum>
	constexpr auto operator&(Flags<Enum> lhs, Enum rhs) -> Flags<Enum>
	{
		return lhs & Flags<Enum>(rhs);
	}

	template<typename Enum>
	constexpr auto operator^(Flags<Enum> lhs, Enum rhs) -> Flags<Enum>
	{
		return lhs ^ Flags<Enum>(rhs);
	}

	template<typename Enum>
	constexpr auto operator+(Flags<Enum> lhs, Enum rhs) -> Flags<Enum>
	{
		return lhs + Flags<Enum>(rhs);
	}

	template<typename Enum>
	constexpr auto operator-(Flags<Enum> lhs, Enum rhs) -> Flags<Enum>
	{
		return lhs - Flags<Enum>(rhs);
	}

	}
}
