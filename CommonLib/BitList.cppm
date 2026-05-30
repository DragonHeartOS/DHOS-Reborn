export module CommonLib:BitList;

import :ArrayList;
import :Math;
import :Error;

export {

	namespace CL {

	struct BitList {
		constexpr BitList() = default;

		constexpr auto push(bool const value) -> void
		{
			if (m_data.is_empty() || m_bits_used == 8) {
				m_data.push(0);
				m_bits_used = 0;
			}

			m_data.get(m_data.size() - 1).unwrap() |= static_cast<u8>(value)
			    << m_bits_used;
			m_bits_used++;
		}

		constexpr auto pop() -> Result<bool, Errors>
		{
			if (size() == 0)
				return Result<bool, Errors>::Err(ErrorsV::PoppingEmptyList {});

			auto const old { test(size() - 1) };
			if (!old)
				return old;

			m_bits_used--;

			if (m_bits_used == 0) {
				m_data.pop();
				if (!m_data.is_empty())
					m_bits_used = 8;
			}

			return Result<bool, Errors>::Ok(*old);
		}

		constexpr auto clear_all() -> void
		{
			m_data.iter().for_each([](u8 &data) { data = 0; });
		}

		constexpr auto set_all() -> void
		{
			m_data.iter().for_each([](u8 &data) { data = 0xff; });
		}

		constexpr auto test(usize index) const -> Result<bool, Errors>
		{
			if (index >= size())
				return Result<bool, Errors>::Err(ErrorsV::InvalidIndex {});

			return Result<bool, Errors>::Ok(
			    (m_data[index / 8] & (1u << (index % 8))) != 0);
		}

		constexpr auto set(usize index) -> Result<void, Errors>
		{
			if (index >= size())
				return Result<void, Errors>::Err(ErrorsV::InvalidIndex {});
			m_data[index / 8] |= static_cast<u8>(1u << (index % 8));
			return Result<void, Errors>::Ok();
		}

		constexpr auto clear(usize index) -> Result<void, Errors>
		{
			if (index >= size())
				return Result<void, Errors>::Err(ErrorsV::InvalidIndex {});
			m_data[index / 8] &= static_cast<u8>(~(1u << (index % 8)));
			return Result<void, Errors>::Ok();
		}

		constexpr auto toggle(usize index) -> Result<void, Errors>
		{
			auto bit = test(index);
			if (!bit)
				return Result<void, Errors>::Err(bit.get_err_unsafe());

			if (*bit)
				return clear(index);
			return set(index);
		}

		constexpr auto reserve(usize size) -> void
		{
			auto const bytes { Math::ceil(static_cast<double>(size) / 8.0) };
			m_data.reserve(static_cast<usize>(bytes));
		}

		constexpr auto is_empty() const -> bool { return size() == 0; }

		constexpr auto size() const -> usize
		{
			if (m_data.is_empty())
				return 0;
			return (m_data.size() - 1) * 8 + static_cast<usize>(m_bits_used);
		}

	private:
		ArrayList<u8> m_data;
		i8 m_bits_used {};
	};

	}
}
