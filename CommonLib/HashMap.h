#pragma once

#include <CommonLib/ArrayList.h>
#include <CommonLib/Error.h>
#include <CommonLib/InitializerList.h>

namespace CL {

/// @brief Check if a type is hashable by checking if it can be hashed using the
/// to_hash function.
/// @tparam T The type to check for hashability.
template<typename T>
concept Hashable = requires(T const &value) { to_hash(value); };

/// @brief A hash map that maps keys of type K to values of type V using
/// separate chaining for collision resolution.
/// @tparam K The type of the keys in the hash map. The keys must be Hashable.
/// @tparam V The type of the values in the hash map.
template<Hashable K, typename V> struct HashMap {
	/// @brief An entry in the hash map, consisting of a key and a value.
	struct Entry {
		K key;
		V value;
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

	/// @brief Get an iterator over the entries in the hash map.
	/// @return An iterator over the entries in the hash map.
	auto iter() -> Iter { return { .map = this }; }
	/// @brief Get an iterator over the entries in the hash map.
	/// @return An iterator over the entries in the hash map.
	auto iter() const -> ConstIter { return { .map = this }; }

	/// @brief Get the number of entries in the hash map.
	/// @return The number of entries in the hash map.
	constexpr auto size() const -> usize { return m_size; }
	/// @brief Check if the hash map is empty.
	/// @return True if the hash map is empty, false otherwise.
	constexpr auto is_empty() const -> bool { return m_size == 0; }

	/// @brief Remove all entries from the hash map.
	auto clear() -> void
	{
		for (usize i {}; i < m_buckets.size(); ++i)
			m_buckets[i].clear();

		m_size = 0;
	}

	/// @brief Check if the hash map contains an entry with a specified key.
	/// @param key The key to check for in the hash map.
	/// @return True if the hash map contains an entry with the specified key,
	/// false otherwise
	auto contains(K const &key) const -> bool
	{
		return find_entry(key) != nullptr;
	}

	/// @brief Get a reference to the value associated with a specified key in
	/// the hash map.
	/// @param key The key to get the value for in the hash map.
	/// @return An Option containing a reference to the value associated with
	/// the specified key if such an entry exists in the hash map, or an empty
	/// Option if no such entry exists.
	auto get(K const &key) -> Option<V &>
	{
		auto *entry = find_entry(key);
		if (entry == nullptr)
			return {};

		return entry->value;
	}
	/// @brief Get a reference to the value associated with a specified key in
	/// the hash map.
	/// @param key The key to get the value for in the hash map.
	/// @return An Option containing a reference to the value associated with
	/// the specified key if such an entry exists in the hash map, or an empty
	/// Option if no such entry exists.
	auto get(K const &key) const -> Option<V const &>
	{
		auto const *entry = find_entry(key);
		if (entry == nullptr)
			return {};

		return entry->value;
	}

	/// @brief Insert a key-value pair into the hash map if the key does not
	/// already exist in the hash map.
	/// @param key The key to insert into the hash map.
	/// @param value The value to associate with the key in the hash map.
	/// @return A Result indicating whether the insertion was successful. The
	/// Result will be Ok if the key was successfully inserted, or Err with an
	/// ErrorsV::HashMapDuplicateKeyError if an entry with the same key already
	/// exists in the hash map.
	auto insert(K key, V value) -> Result<void, Errors>
	{
		if (find_entry(key) != nullptr)
			return Result<void, Errors>::Err(
			    ErrorsV::HashMapDuplicateKeyError {});

		ensure_capacity_for_insert();
		insert_entry(move(key), move(value));

		return Result<void, Errors>::Ok();
	}

	/// @brief Insert a key-value pair into the hash map, replacing any existing
	/// entry with the same key.
	/// @param key The key to insert into the hash map.
	/// @param value The value to associate with the key in the hash map. If an
	/// entry with the same key already exists in the hash map, its value will
	/// be replaced with the new value.
	auto insert_or_replace(K key, V value) -> void
	{
		if (auto *entry = find_entry(key); entry != nullptr) {
			entry->value = move(value);
			return;
		}

		ensure_capacity_for_insert();
		insert_entry(move(key), move(value));
	}

	/// @brief Remove the entry with a specified key from the hash map.
	/// @param key The key of the entry to remove from the hash map.
	/// @return An Option containing the value associated with the removed key
	/// if such an entry existed in the hash map, or an empty Option if no such
	/// entry existed.
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
	struct Location {
		usize bucket_index {};
		usize entry_index {};
	};

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
