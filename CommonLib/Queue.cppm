export module CommonLib:Queue;

import :Atomic;
import :CString;
import :Option;
import :Types;

export {
	namespace CL {

	template<typename T, usize Capacity> class MpscQueue {
		static_assert(Capacity > 0, "queue capacity must be non-zero");
		static_assert((Capacity & (Capacity - 1)) == 0,
		    "queue capacity must be a power of two");
		static_assert(__is_trivially_copyable(T),
		    "MpscQueue requires trivially copyable elements");

	public:
		MpscQueue() { init(); }

		auto init() -> void
		{
			m_head.store(0, MemoryOrder::Relaxed);
			m_tail = 0;
			for (usize i {}; i < Capacity; ++i)
				m_slots[i].seq.store(i, MemoryOrder::Relaxed);
		}

		auto push_blocking(T const &value) -> void
		{
			for (;;) {
				u64 head { m_head.load(MemoryOrder::Relaxed) };
				auto &slot { m_slots[head & mask] };
				u64 seq { slot.seq.load(MemoryOrder::Acquire) };
				i64 dif { static_cast<i64>(seq) - static_cast<i64>(head) };

				if (dif == 0) {
					u64 expected { head };
					if (!m_head.compare_exchange(expected, head + 1,
					        MemoryOrder::AcquireRelease,
					        MemoryOrder::Relaxed)) {
						continue;
					}

					memcpy(slot.storage, &value, sizeof(T));
					slot.seq.store(head + 1, MemoryOrder::Release);
					return;
				}

				continue;
			}
		}

		auto try_pop() -> Option<T>
		{
			auto &slot { m_slots[m_tail & mask] };
			u64 const expected { m_tail + 1 };

			if (slot.seq.load(MemoryOrder::Acquire) != expected)
				return {};

			alignas(T) unsigned char raw[sizeof(T)];
			memcpy(raw, slot.storage, sizeof(T));
			Option<T> out { *reinterpret_cast<T const *>(raw) };
			slot.seq.store(m_tail + Capacity, MemoryOrder::Release);
			++m_tail;
			return out;
		}

		template<typename Fn> auto drain(Fn &&fn) -> void
		{
			while (auto value = try_pop())
				fn(*value);
		}

	private:
		struct Slot {
			Atomic<u64> seq { 0 };
			alignas(T) unsigned char storage[sizeof(T)] {};
		};

		static constexpr u64 mask { Capacity - 1 };

		Atomic<u64> m_head { 0 };
		u64 m_tail {};
		Slot m_slots[Capacity] {};
	};

	}
}
