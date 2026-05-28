#pragma once

#include <CommonLib/ArrayList.h>
#include <CommonLib/Error.h>
#include <CommonLib/InitializerList.h>

namespace CL {

template<typename T>
concept Hashable = requires(T const &value) { to_hash(value); };

template<Hashable K, typename V> struct HashMap {
	struct Entry {
		K key;
		V value;
	};

	struct Location {
		usize bucket_index {};
		usize entry_index {};
	};

	struct Iter : Iterator<Iter> {
		HashMap *map { nullptr };
		usize bucket_index {};
		usize entry_index {};

		auto next() -> Option<Entry &>
		{
			if (map == nullptr)
				return {};

			while (bucket_index < map->m_buckets.size()) {
				auto &bucket = map->m_buckets[bucket_index];

				if (entry_index < bucket.size())
					return bucket[entry_index++];

				++bucket_index;
				entry_index = 0;
			}

			return {};
		}
	};

	struct ConstIter : Iterator<ConstIter> {
		HashMap const *map { nullptr };
		usize bucket_index {};
		usize entry_index {};

		auto next() -> Option<Entry const &>
		{
			if (map == nullptr)
				return {};

			while (bucket_index < map->m_buckets.size()) {
				auto const &bucket = map->m_buckets[bucket_index];

				if (entry_index < bucket.size())
					return bucket[entry_index++];

				++bucket_index;
				entry_index = 0;
			}

			return {};
		}
	};

	HashMap() { init_buckets(InitialBucketCount); }

	HashMap(InitializerList<Entry> init)
	    : HashMap()
	{
		for (usize i {}; i < init.size(); ++i)
			insert_or_replace(init[i].key, init[i].value);
	}

	auto iter() -> Iter { return { .map = this }; }

	auto iter() const -> ConstIter { return { .map = this }; }

	constexpr auto size() const -> usize { return m_size; }
	constexpr auto is_empty() const -> bool { return m_size == 0; }

	auto clear() -> void
	{
		for (usize i {}; i < m_buckets.size(); ++i)
			m_buckets[i].clear();

		m_size = 0;
	}

	auto contains(K const &key) const -> bool
	{
		return find_entry(key) != nullptr;
	}

	auto get(K const &key) -> Option<V &>
	{
		auto *entry = find_entry(key);
		if (entry == nullptr)
			return {};

		return entry->value;
	}

	auto get(K const &key) const -> Option<V const &>
	{
		auto const *entry = find_entry(key);
		if (entry == nullptr)
			return {};

		return entry->value;
	}

	auto insert(K key, V value) -> Result<void, Errors>
	{
		if (find_entry(key) != nullptr)
			return Result<void, Errors>::Err(Errors::make<0>());

		ensure_capacity_for_insert();
		insert_entry(move(key), move(value));

		return Result<void, Errors>::Ok();
	}

	auto insert_or_replace(K key, V value) -> void
	{
		if (auto *entry = find_entry(key); entry != nullptr) {
			entry->value = move(value);
			return;
		}

		ensure_capacity_for_insert();
		insert_entry(move(key), move(value));
	}

	auto remove(K const &key) -> Option<V>
	{
		auto position = find_position(key);
		if (!position)
			return {};

		auto const &location = *position;
		auto &bucket = m_buckets[location.bucket_index];

		auto value = move(bucket[location.entry_index].value);
		bucket.remove_at(location.entry_index);
		--m_size;

		return Option<V>(move(value));
	}

private:
	static constexpr usize InitialBucketCount = 8;

	using Bucket = ArrayList<Entry>;

	static auto bucket_index_for(K const &key, usize bucket_count) -> usize
	{
		return to_hash(key) % bucket_count;
	}

	auto bucket_index_for(K const &key) const -> usize
	{
		return bucket_index_for(key, m_buckets.size());
	}

	static auto find_entry_index(Bucket const &bucket, K const &key)
	    -> Option<usize>
	{
		for (usize i {}; i < bucket.size(); ++i) {
			if (bucket[i].key == key)
				return i;
		}

		return {};
	}

	auto find_position(K const &key) const -> Option<Location>
	{
		usize bucket_index = bucket_index_for(key);
		auto const &bucket = m_buckets[bucket_index];

		auto entry_index = find_entry_index(bucket, key);
		if (!entry_index)
			return {};

		return Location {
			.bucket_index = bucket_index,
			.entry_index = *entry_index,
		};
	}

	template<typename Self>
	static auto find_entry_impl(Self &self, K const &key)
	    -> decltype(&self.m_buckets[0][0])
	{
		auto position = self.find_position(key);
		if (!position)
			return nullptr;

		auto const &location = *position;
		return &self.m_buckets[location.bucket_index][location.entry_index];
	}

	auto find_entry(K const &key) -> Entry *
	{
		return find_entry_impl(*this, key);
	}

	auto find_entry(K const &key) const -> Entry const *
	{
		return find_entry_impl(*this, key);
	}

	auto insert_entry(K key, V value) -> void
	{
		usize index = bucket_index_for(key);

		m_buckets[index].push(Entry {
		    .key = move(key),
		    .value = move(value),
		});

		++m_size;
	}

	auto ensure_capacity_for_insert() -> void
	{
		if ((m_size + 1) * 4 <= m_buckets.size() * 3)
			return;

		rehash(m_buckets.size() * 2);
	}

	auto rehash(usize new_bucket_count) -> void
	{
		if (new_bucket_count < InitialBucketCount)
			new_bucket_count = InitialBucketCount;

		ArrayList<Bucket> new_buckets;
		new_buckets.reserve(new_bucket_count);

		for (usize i {}; i < new_bucket_count; ++i)
			new_buckets.emplace();

		for (usize i {}; i < m_buckets.size(); ++i) {
			auto &bucket = m_buckets[i];

			for (usize j {}; j < bucket.size(); ++j) {
				auto &entry = bucket[j];
				usize index = bucket_index_for(entry.key, new_bucket_count);

				new_buckets[index].push(Entry {
				    .key = move(entry.key),
				    .value = move(entry.value),
				});
			}
		}

		m_buckets = move(new_buckets);
	}

	auto init_buckets(usize count) -> void
	{
		m_buckets.reserve(count);

		for (usize i {}; i < count; ++i)
			m_buckets.emplace();
	}

	ArrayList<Bucket> m_buckets;
	usize m_size { 0 };
};

}
