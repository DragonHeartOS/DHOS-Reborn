module;

#include <stdint.h>

export module CommonLib:BitArray;

import :Option;
import :Types;

export {
	namespace CL {

	template<usize N> struct BitArray {
		static_assert(N > 0, "BitArray size must be non-zero");

#if INTPTR_MAX == INT64_MAX
		using Word = u64;
#else
		using Word = u32;
#endif

		static constexpr usize word_bits { sizeof(Word) * 8 };
		static constexpr usize word_count { (N + word_bits - 1) / word_bits };
		static constexpr Word full_word { static_cast<Word>(~Word { 0 }) };
		static constexpr usize last_bits { N % word_bits };
		static constexpr Word last_mask = []() constexpr -> Word {
			if constexpr (last_bits == 0)
				return full_word;

			return static_cast<Word>((Word { 1 } << last_bits) - 1);
		}();

		constexpr BitArray() = default;

		constexpr auto size() const -> usize { return N; }
		constexpr auto words() -> Word * { return m_words; }
		constexpr auto words() const -> Word const * { return m_words; }

		auto clear_all() -> void
		{
			for (usize i {}; i < word_count; ++i)
				m_words[i] = 0;
		}

		auto set_all() -> void
		{
			for (usize i {}; i < word_count; ++i)
				m_words[i] = full_word;

			m_words[word_count - 1] &= last_mask;
		}

		constexpr auto test(usize index) const -> bool
		{
			if (index >= N)
				return false;

			return (m_words[word_index(index)] & bit_mask(index)) != 0;
		}

		auto set(usize index) -> void
		{
			if (index >= N)
				return;

			m_words[word_index(index)] |= bit_mask(index);
		}

		auto clear(usize index) -> void
		{
			if (index >= N)
				return;

			m_words[word_index(index)] &= static_cast<Word>(~bit_mask(index));
		}

		auto toggle(usize index) -> void
		{
			if (index >= N)
				return;

			m_words[word_index(index)] ^= bit_mask(index);
		}

		auto set_range(usize start, usize count) -> void
		{
			range_apply(
			    start, count, [](Word &word, Word mask) { word |= mask; });
		}

		auto clear_range(usize start, usize count) -> void
		{
			range_apply(start, count, [](Word &word, Word mask) {
				word &= static_cast<Word>(~mask);
			});
		}

		auto toggle_range(usize start, usize count) -> void
		{
			range_apply(
			    start, count, [](Word &word, Word mask) { word ^= mask; });
		}

		constexpr auto test_range(usize start, usize count) const -> bool
		{
			if (count == 0)
				return true;
			if (start >= N)
				return false;

			usize end = start + count;
			if (end < start || end > N)
				end = N;

			usize start_word = start / word_bits;
			usize end_word = (end - 1) / word_bits;

			for (usize word_index {}; word_index <= end_word; ++word_index) {
				usize begin = word_index == start_word ? start % word_bits : 0;
				usize finish
				    = word_index == end_word ? end % word_bits : word_bits;
				if (finish == 0)
					finish = word_bits;
				Word mask = range_mask(begin, finish);
				if ((m_words[word_index] & mask) != mask)
					return false;
			}

			return true;
		}

		auto find_first_set() const -> Option<usize>
		{
			for (usize word_index {}; word_index < word_count; ++word_index) {
				Word word = m_words[word_index];
				if (word == 0)
					continue;

				return word_index * word_bits + first_bit_index(word);
			}

			return {};
		}

		auto find_first_clear() const -> Option<usize>
		{
			for (usize word_index {}; word_index < word_count; ++word_index) {
				Word word = static_cast<Word>(~m_words[word_index]);
				if (word_index == word_count - 1)
					word &= last_mask;

				if (word == 0)
					continue;

				return word_index * word_bits + first_bit_index(word);
			}

			return {};
		}

	private:
		static constexpr auto word_index(usize index) -> usize
		{
			return index / word_bits;
		}

		static constexpr auto bit_mask(usize index) -> Word
		{
			return static_cast<Word>(Word { 1 } << (index % word_bits));
		}

		static constexpr auto range_mask(usize begin, usize end) -> Word
		{
			if (begin >= end)
				return 0;

			Word left = begin == 0 ? full_word
			                       : static_cast<Word>(full_word << begin);
			Word right = end == word_bits
			    ? full_word
			    : static_cast<Word>((Word { 1 } << end) - 1);
			return static_cast<Word>(left & right);
		}

		template<typename Fn>
		auto range_apply(usize start, usize count, Fn &&fn) -> void
		{
			if (count == 0 || start >= N)
				return;

			usize end = start + count;
			if (end < start || end > N)
				end = N;

			usize start_word = start / word_bits;
			usize end_word = (end - 1) / word_bits;

			for (usize word_index = start_word; word_index <= end_word;
			    ++word_index) {
				usize begin = word_index == start_word ? start % word_bits : 0;
				usize finish
				    = word_index == end_word ? end % word_bits : word_bits;
				if (finish == 0)
					finish = word_bits;
				Word mask = range_mask(begin, finish);
				fn(m_words[word_index], mask);
			}

			m_words[word_count - 1] &= last_mask;
		}

		static auto first_bit_index(Word word) -> usize
		{
			usize index {};
			while ((word & Word { 1 }) == 0) {
				word >>= 1;
				++index;
			}

			return index;
		}

		Word m_words[word_count] {};
	};

	}
}
