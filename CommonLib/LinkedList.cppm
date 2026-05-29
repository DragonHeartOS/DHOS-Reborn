export module CommonLib:LinkedList;

import :InitializerList;
import :Iterator;
import :Option;
import :Types;
import :Utility;

export {
	namespace CL {

	/// @brief A doubly linked list of elements of type T.
	/// @tparam T The type of the elements in the linked list.
	template<typename T> struct LinkedList {
	private:
		struct Node {
			T value;
			Node *next { nullptr };
			Node *prev { nullptr };

			template<typename... Args>
			Node(Args &&...args)
			    : value { static_cast<Args &&>(args)... }
			{
			}
		};

	public:
		struct Iter : Iterator<Iter> {
			Node *front { nullptr };
			Node *back { nullptr };

			auto next() -> Option<T &>
			{
				if (!front)
					return {};

				Node *node = front;

				if (front == back) {
					front = nullptr;
					back = nullptr;
				} else {
					front = front->next;
				}

				return node->value;
			}

			auto next_back() -> Option<T &>
			{
				if (!back)
					return {};

				Node *node = back;

				if (front == back) {
					front = nullptr;
					back = nullptr;
				} else {
					back = back->prev;
				}

				return node->value;
			}
		};

		struct ConstIter : Iterator<ConstIter> {
			Node const *current { nullptr };
			Node const *back { nullptr };

			auto next() -> Option<T const &>
			{
				if (!current)
					return {};

				Node const *node = current;
				current = current->next;

				return node->value;
			}

			auto next_back() -> Option<T const &>
			{
				if (!back)
					return {};

				Node const *node = back;

				if (current == back) {
					current = nullptr;
					back = nullptr;
				} else {
					back = back->prev;
				}

				return node->value;
			}
		};

		LinkedList() = default;

		LinkedList(LinkedList const &other)
		{
			for (Node *node = other.m_head; node; node = node->next)
				push(node->value);
		}
		LinkedList(LinkedList &&other)
		    : m_head { other.m_head }
		    , m_tail { other.m_tail }
		    , m_size { other.m_size }
		{
			other.m_head = nullptr;
			other.m_tail = nullptr;
			other.m_size = 0;
		}

		LinkedList(InitializerList<T> init)
		{
			for (usize i = 0; i < init.size(); ++i)
				emplace(init[i]);
		}

		~LinkedList() { clear(); }

		auto operator=(LinkedList const &other) -> LinkedList &
		{
			if (this == &other)
				return *this;

			clear();

			for (Node *node = other.m_head; node; node = node->next)
				push(node->value);

			return *this;
		}
		auto operator=(LinkedList &&other) -> LinkedList &
		{
			if (this == &other)
				return *this;

			clear();

			m_head = other.m_head;
			m_tail = other.m_tail;
			m_size = other.m_size;

			other.m_head = nullptr;
			other.m_tail = nullptr;
			other.m_size = 0;

			return *this;
		}

		/// @brief Construct a value in place at the end of the linked list.
		/// @tparam Args The types of the arguments to construct the value.
		/// @param args The arguments to construct the value.
		/// @return A reference to the constructed value.
		template<typename... Args> auto emplace(Args &&...args) -> T &
		{
			Node *node = new Node(static_cast<Args &&>(args)...);

			if (!m_head) {
				m_head = node;
				m_tail = node;
			} else {
				node->prev = m_tail;
				m_tail->next = node;
				m_tail = node;
			}

			++m_size;
			return node->value;
		}

		/// @brief Push a value at the end of the linked list.
		/// @param value The value to push.
		auto push(T const &value) -> void { emplace(value); }
		/// @brief Push a value at the end of the linked list.
		/// @param value The value to push.
		auto push(T &&value) -> void { emplace(static_cast<T &&>(value)); }

		/// @brief Pop a value from the end of the linked list.
		auto pop() -> void
		{
			if (!m_tail)
				return;

			Node *tail = m_tail;

			m_tail = tail->prev;

			if (m_tail)
				m_tail->next = nullptr;
			else
				m_head = nullptr;

			delete tail;
			--m_size;
		}

		/// @brief Clear the linked list, removing all elements.
		auto clear() -> void
		{
			Node *node = m_head;

			while (node) {
				Node *next = node->next;
				delete node;
				node = next;
			}

			m_head = nullptr;
			m_tail = nullptr;
			m_size = 0;
		}

		/// @brief Get the number of elements in the linked list.
		/// @return The number of elements in the linked list.
		constexpr auto size() const -> usize { return m_size; }
		/// @brief Check if the linked list is empty.
		/// @return True if the linked list is empty, false otherwise.
		constexpr auto is_empty() const -> bool { return m_size == 0; }

		/// @brief Get a reference to the first element in the linked list.
		/// @return An Option containing a reference to the first element in the
		/// linked list if it is not empty, or an empty Option if the linked
		/// list is empty.
		auto first() -> Option<T &>
		{
			if (!m_head)
				return {};

			return m_head->value;
		}
		/// @brief Get a reference to the first element in the linked list.
		/// @return An Option containing a reference to the first element in the
		/// linked list if it is not empty, or an empty Option if the linked
		/// list is empty.
		auto first() const -> Option<T const &>
		{
			if (!m_head)
				return {};

			return m_head->value;
		}

		/// @brief Get a reference to the last element in the linked list.
		/// @return An Option containing a reference to the last element in the
		/// linked list if it is not empty, or an empty Option if the linked
		/// list is empty.
		auto last() -> Option<T &>
		{
			if (!m_tail)
				return {};

			return m_tail->value;
		}
		/// @brief Get a reference to the last element in the linked list.
		/// @return An Option containing a reference to the last element in the
		/// linked list if it is not empty, or an empty Option if the linked
		/// list is empty.
		auto last() const -> Option<T const &>
		{
			if (!m_tail)
				return {};

			return m_tail->value;
		}

		/// @brief Get an iterator over the elements in the linked list.
		/// @return An iterator over the elements in the linked list.
		auto iter() -> Iter { return Iter { .front = m_head, .back = m_tail }; }
		/// @brief Get an iterator over the elements in the linked list.
		/// @return An iterator over the elements in the linked list.
		auto iter() const -> ConstIter
		{
			return ConstIter { .current = m_head, .back = m_tail };
		}

	private:
		Node *m_head { nullptr };
		Node *m_tail { nullptr };
		usize m_size { 0 };
	};

	}
}
