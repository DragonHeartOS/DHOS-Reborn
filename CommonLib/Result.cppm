export module CommonLib:Result;

import :Option;
import :Platform;
import :StringView;
import :TypeTraits;

export {
	namespace CL {

	template<typename T, typename E> struct Result;

	namespace detail {

	struct ResultOkTag { };
	struct ResultErrTag { };
	enum class ResultState : bool {
		Err = false,
		Ok = true,
	};

	constexpr auto require_ok(bool is_ok, StringView message) -> void
	{
		if (!is_ok)
			panic(message);
	}

	constexpr auto require_err(bool is_ok, StringView message) -> void
	{
		if (is_ok)
			panic(message);
	}

	template<typename T> constexpr auto move_ref(T &value) -> T &&
	{
		return static_cast<T &&>(value);
	}

	template<typename T> struct ResultConstRef {
		using Type = AddConstT<RemoveReferenceT<T>> &;
	};

	template<typename T>
	using ResultConstRefT = typename ResultConstRef<T>::Type;

	template<typename Derived, typename T, typename E> struct ResultAccessBase {
		constexpr auto unwrap() & -> T & { return unwrap_impl(); }
		constexpr auto unwrap() const & -> ResultConstRefT<T>
		{
			return unwrap_const_impl();
		}
		constexpr auto unwrap() && -> T && { return move_ref(unwrap_impl()); }

		constexpr auto expect(StringView const message) & -> T &
		{
			return expect_impl(message);
		}

		constexpr auto expect(
		    StringView const message) const & -> ResultConstRefT<T>
		{
			return expect_const_impl(message);
		}

		constexpr auto unwrap_err() & -> E &
		{
			return unwrap_err_impl(static_cast<Derived &>(*this));
		}
		constexpr auto unwrap_err() const & -> E const &
		{
			return unwrap_err_const_impl(static_cast<Derived const &>(*this));
		}
		constexpr auto unwrap_err() && -> E &&
		{
			auto &self = static_cast<Derived &>(*this);
			return move_ref(unwrap_err_impl(self));
		}

		constexpr auto operator*() & -> T & { return unwrap(); }
		constexpr auto operator*() const & -> ResultConstRefT<T>
		{
			return unwrap();
		}

		constexpr auto operator->() -> RemoveReferenceT<T> *
		{
			return &unwrap();
		}
		constexpr auto operator->() const -> RemoveReferenceT<T> const *
		{
			return &unwrap();
		}

	protected:
		constexpr auto unwrap_impl() -> T &
		{
			auto &self = static_cast<Derived &>(*this);
			require_ok(self.is_ok(), "Attempt to unwrap Err Result");
			return self.get_ok_unsafe();
		}

		constexpr auto expect_impl(StringView const message) -> T &
		{
			auto &self = static_cast<Derived &>(*this);
			require_ok(self.is_ok(), message);
			return self.get_ok_unsafe();
		}

		constexpr auto unwrap_const_impl() const -> ResultConstRefT<T>
		{
			auto const &self = static_cast<Derived const &>(*this);
			require_ok(self.is_ok(), "Attempt to unwrap Err Result");
			return self.get_ok_unsafe();
		}

		constexpr auto expect_const_impl(StringView const message) const
		    -> ResultConstRefT<T>
		{
			auto const &self = static_cast<Derived const &>(*this);
			require_ok(self.is_ok(), message);
			return self.get_ok_unsafe();
		}

		constexpr auto unwrap_err_impl(Derived &self) -> E &
		{
			require_err(self.is_ok(), "Attempt to unwrap_err Ok Result");
			return self.get_err_unsafe();
		}

		constexpr auto unwrap_err_const_impl(Derived const &self) const
		    -> E const &
		{
			require_err(self.is_ok(), "Attempt to unwrap_err Ok Result");
			return self.get_err_unsafe();
		}
	};

	template<typename T, typename E> struct ResultStorage {
		union Storage {
			constexpr Storage() { }
			constexpr ~Storage() { }

			T ok;
			E err;
		} data;

		ResultState state;

		constexpr auto is_ok() const -> bool
		{
			return state == ResultState::Ok;
		}

		constexpr auto construct_ok(T value) -> void
		{
			new (&data.ok) T(move_ref(value));
			state = ResultState::Ok;
		}

		constexpr auto construct_err(E error) -> void
		{
			new (&data.err) E(move_ref(error));
			state = ResultState::Err;
		}

		constexpr auto destroy_active() -> void
		{
			if (is_ok())
				data.ok.~T();
			else
				data.err.~E();
		}

		constexpr auto copy_from(ResultStorage const &other) -> void
		{
			if (other.is_ok())
				construct_ok(other.data.ok);
			else
				construct_err(other.data.err);
		}

		constexpr auto move_from(ResultStorage &&other) -> void
		{
			if (other.is_ok())
				construct_ok(move_ref(other.data.ok));
			else
				construct_err(move_ref(other.data.err));
		}
	};

	template<typename E> struct ResultStorage<void, E> {
		union Storage {
			constexpr Storage() { }
			constexpr ~Storage() { }

			char dummy;
			E err;
		} data;

		ResultState state;

		constexpr auto is_ok() const -> bool
		{
			return state == ResultState::Ok;
		}

		constexpr auto construct_ok() -> void
		{
			data.dummy = 0;
			state = ResultState::Ok;
		}

		constexpr auto construct_err(E error) -> void
		{
			new (&data.err) E(move_ref(error));
			state = ResultState::Err;
		}

		constexpr auto destroy_active() -> void
		{
			if (!is_ok())
				data.err.~E();
		}

		constexpr auto copy_from(ResultStorage const &other) -> void
		{
			if (other.is_ok())
				construct_ok();
			else
				construct_err(other.data.err);
		}

		constexpr auto move_from(ResultStorage &&other) -> void
		{
			if (other.is_ok())
				construct_ok();
			else
				construct_err(move_ref(other.data.err));
		}
	};

	template<typename Storage> struct ResultOwnershipBase {
	protected:
		constexpr ResultOwnershipBase()
		    : m_storage {}
		{
		}

		constexpr ResultOwnershipBase(ResultOwnershipBase const &other)
		    : m_storage {}
		{
			m_storage.copy_from(other.m_storage);
		}

		constexpr ResultOwnershipBase(ResultOwnershipBase &&other)
		    : m_storage {}
		{
			m_storage.move_from(move_ref(other.m_storage));
		}

		constexpr ~ResultOwnershipBase() { m_storage.destroy_active(); }

		constexpr auto assign_copy(ResultOwnershipBase const &other) -> void
		{
			if (this == &other)
				return;

			m_storage.destroy_active();
			m_storage.copy_from(other.m_storage);
		}

		constexpr auto assign_move(ResultOwnershipBase &&other) -> void
		{
			if (this == &other)
				return;

			m_storage.destroy_active();
			m_storage.move_from(move_ref(other.m_storage));
		}

	public:
		constexpr auto storage() -> Storage & { return m_storage; }
		constexpr auto storage() const -> Storage const & { return m_storage; }

		Storage m_storage;
	};

	template<typename Derived, typename T, typename E> struct ResultBase {
		constexpr auto self() -> Derived &
		{
			return static_cast<Derived &>(*this);
		}
		constexpr auto self() const -> Derived const &
		{
			return static_cast<Derived const &>(*this);
		}

		constexpr auto is_ok() -> bool { return self().storage().is_ok(); }
		constexpr auto is_ok() const -> bool
		{
			return self().storage().is_ok();
		}

		constexpr auto is_err() -> bool { return !self().is_ok(); }
		constexpr auto is_err() const -> bool { return !self().is_ok(); }

		constexpr explicit operator bool() const
		{
			return self().storage().is_ok();
		}

		template<typename Fn> constexpr auto is_ok_and(Fn &&fn) -> bool
		{
			return is_ok_and_impl(static_cast<Fn &&>(fn));
		}
		template<typename Fn> constexpr auto is_ok_and(Fn &&fn) const -> bool
		{
			return is_ok_and_impl(static_cast<Fn &&>(fn));
		}

		template<typename Fn> constexpr auto is_err_and(Fn &&fn) -> bool
		{
			return is_err_and_impl(static_cast<Fn &&>(fn));
		}
		template<typename Fn> constexpr auto is_err_and(Fn &&fn) const -> bool
		{
			return is_err_and_impl(static_cast<Fn &&>(fn));
		}

		constexpr auto ok() const & -> Option<T> { return ok_impl(); }
		constexpr auto ok() && -> Option<T> { return ok_move_impl(); }

		constexpr auto err() const & -> Option<E> { return err_impl(); }
		constexpr auto err() && -> Option<E> { return err_move_impl(); }

		constexpr auto unwrap_or(T fallback) const & -> T
		{
			return unwrap_or_impl(fallback);
		}
		constexpr auto unwrap_or(T fallback) && -> T
		{
			return unwrap_or_move_impl(fallback);
		}

		template<typename Fn>
		constexpr auto unwrap_or_else(Fn &&fn) const & -> T
		{
			if (self().is_ok())
				return self().get_ok_unsafe();

			return fn(self().get_err_unsafe());
		}

		template<typename Fn> constexpr auto map(Fn &&fn) const &
		{
			using U = decltype(fn(self().get_ok_unsafe()));

			if (self().is_err())
				return Result<U, E>::Err(self().get_err_unsafe());

			return Result<U, E>::Ok(fn(self().get_ok_unsafe()));
		}

		template<typename Fn> constexpr auto map_err(Fn &&fn) const &
		{
			using F = decltype(fn(self().get_err_unsafe()));

			if (self().is_ok())
				return Result<T, F>::Ok(self().get_ok_unsafe());

			return Result<T, F>::Err(fn(self().get_err_unsafe()));
		}

		template<typename Fn> constexpr auto and_then(Fn &&fn) const &
		{
			using Ret = decltype(fn(self().get_ok_unsafe()));

			if (self().is_err())
				return Ret::Err(self().get_err_unsafe());

			return fn(self().get_ok_unsafe());
		}

		template<typename Fn> constexpr Derived &inspect(Fn &&fn)
		{
			if (self().is_ok())
				fn(self().get_ok_unsafe());

			return self();
		}

		template<typename Fn> constexpr Derived &inspect_err(Fn &&fn)
		{
			if (self().is_err())
				fn(self().get_err_unsafe());

			return self();
		}

	protected:
		template<typename Fn>
		constexpr auto is_ok_and_impl(Fn &&fn) const -> bool
		{
			return self().is_ok() && fn(self().get_ok_unsafe());
		}

		template<typename Fn>
		constexpr auto is_err_and_impl(Fn &&fn) const -> bool
		{
			return self().is_err() && fn(self().get_err_unsafe());
		}

		constexpr auto ok_impl() const -> Option<T>
		{
			if (self().is_err())
				return {};

			return Option<T>(self().get_ok_unsafe());
		}

		constexpr auto ok_move_impl() -> Option<T>
		{
			if (self().is_err())
				return {};

			return Option<T>(move_ref(self().get_ok_unsafe()));
		}

		constexpr auto err_impl() const -> Option<E>
		{
			if (self().is_ok())
				return {};

			return Option<E>(self().get_err_unsafe());
		}

		constexpr auto err_move_impl() -> Option<E>
		{
			if (self().is_ok())
				return {};

			return Option<E>(move_ref(self().get_err_unsafe()));
		}

		constexpr auto unwrap_or_impl(T fallback) const -> T
		{
			if (self().is_ok())
				return self().get_ok_unsafe();

			return fallback;
		}

		constexpr auto unwrap_or_move_impl(T fallback) -> T
		{
			if (self().is_ok())
				return move_ref(self().get_ok_unsafe());

			return fallback;
		}
	};

	template<typename Derived, typename E> struct ResultVoidAccessBase {
		constexpr auto is_ok() -> bool
		{
			return static_cast<Derived &>(*this).storage().is_ok();
		}

		constexpr auto is_ok() const -> bool
		{
			return static_cast<Derived const &>(*this).storage().is_ok();
		}

		constexpr auto is_err() -> bool { return !is_ok(); }
		constexpr auto is_err() const -> bool { return !is_ok(); }

		constexpr explicit operator bool() const { return is_ok(); }

		constexpr auto unwrap() const -> void { unwrap_impl(); }
		constexpr auto expect(StringView const message) const -> void
		{
			expect_impl(message);
		}

		constexpr auto unwrap_err() & -> E & { return unwrap_err_impl(); }
		constexpr auto unwrap_err() const & -> E const &
		{
			return unwrap_err_const_impl();
		}
		constexpr auto unwrap_err() && -> E &&
		{
			return move_ref(unwrap_err_impl());
		}

		constexpr auto err() const & -> Option<E> { return err_impl(); }
		constexpr auto err() && -> Option<E> { return err_move_impl(); }

	protected:
		constexpr auto unwrap_impl() const -> void
		{
			auto const &self = static_cast<Derived const &>(*this);
			require_ok(self.storage().is_ok(), "Attempt to unwrap Err Result");
		}

		constexpr auto expect_impl(StringView const message) const -> void
		{
			auto const &self = static_cast<Derived const &>(*this);
			require_ok(self.storage().is_ok(), message);
		}

		constexpr auto unwrap_err_impl() -> E &
		{
			auto &self = static_cast<Derived &>(*this);
			require_err(
			    self.storage().is_ok(), "Attempt to unwrap_err Ok Result");
			return self.storage().data.err;
		}

		constexpr auto unwrap_err_const_impl() const -> E const &
		{
			auto const &self = static_cast<Derived const &>(*this);
			require_err(
			    self.storage().is_ok(), "Attempt to unwrap_err Ok Result");
			return self.storage().data.err;
		}

		constexpr auto err_impl() const -> Option<E>
		{
			auto const &self = static_cast<Derived const &>(*this);
			if (self.storage().is_ok())
				return {};

			return Option<E>(self.storage().data.err);
		}

		constexpr auto err_move_impl() -> Option<E>
		{
			auto &self = static_cast<Derived &>(*this);
			if (self.storage().is_ok())
				return {};

			return Option<E>(move_ref(self.storage().data.err));
		}
	};

	}

	/// @brief A type that represents either a success (Ok) or an error (Err)
	/// value.
	/// @tparam T The type of the success value.
	/// @tparam E The type of the error value.
	template<typename T, typename E>
	struct Result
	    : detail::ResultOwnershipBase<detail::ResultStorage<T, E>>
	    , detail::ResultBase<Result<T, E>, T, E>
	    , detail::ResultAccessBase<Result<T, E>, T, E> {
		/// @brief Create a new Result with an Ok value.
		/// @param value The value to store in the Ok variant of the Result.
		/// @return A new Result with the specified Ok value.
		static constexpr auto Ok(T value) -> Result
		{
			return Result(detail::ResultOkTag {}, detail::move_ref(value));
		}

		/// @brief Create a new Result with an Err value.
		/// @param error The error to store in the Err variant of the Result.
		/// @return A new Result with the specified Err value.
		static constexpr auto Err(E error) -> Result
		{
			return Result(detail::ResultErrTag {}, detail::move_ref(error));
		}

		constexpr Result(Result const &other)
		    : detail::ResultOwnershipBase<detail::ResultStorage<T, E>>(other)
		{
		}
		constexpr Result(Result &&other)
		    : detail::ResultOwnershipBase<detail::ResultStorage<T, E>>(
		          detail::move_ref(other))
		{
		}

		constexpr auto operator=(Result const &other) -> Result &
		{
			this->assign_copy(other);
			return *this;
		}

		constexpr auto operator=(Result &&other) -> Result &
		{
			this->assign_move(detail::move_ref(other));
			return *this;
		}

		/// @brief Get a reference to the success value in the Result without
		/// checking if it is an Ok or Err variant.
		constexpr auto get_ok_unsafe() -> T &
		{
			return this->m_storage.data.ok;
		}
		constexpr auto get_ok_unsafe() const -> T const &
		{
			return this->m_storage.data.ok;
		}

		/// @brief Get a reference to the error value in the Result without
		/// checking if it is an Ok or Err variant.
		constexpr auto get_err_unsafe() -> E &
		{
			return this->m_storage.data.err;
		}
		constexpr auto get_err_unsafe() const -> E const &
		{
			return this->m_storage.data.err;
		}

	private:
		constexpr Result(detail::ResultOkTag, T value)
		    : detail::ResultOwnershipBase<detail::ResultStorage<T, E>>()
		{
			this->m_storage.construct_ok(detail::move_ref(value));
		}

		constexpr Result(detail::ResultErrTag, E error)
		    : detail::ResultOwnershipBase<detail::ResultStorage<T, E>>()
		{
			this->m_storage.construct_err(detail::move_ref(error));
		}
	};

	/// @brief A specialization of the Result type for the case where the
	/// success value is void.
	/// @tparam E The type of the error value.
	template<typename E>
	struct Result<void, E>
	    : detail::ResultOwnershipBase<detail::ResultStorage<void, E>>
	    , detail::ResultVoidAccessBase<Result<void, E>, E> {
		/// @brief Create a new Result with an Ok value.
		/// @return A new Result with the Ok variant.
		static constexpr auto Ok() -> Result
		{
			return Result(detail::ResultOkTag {});
		}

		/// @brief Create a new Result with an Err value.
		/// @param error The error to store in the Err variant of the Result.
		/// @return A new Result with the specified Err value.
		static constexpr auto Err(E error) -> Result
		{
			return Result(detail::ResultErrTag {}, detail::move_ref(error));
		}

		constexpr Result(Result const &other)
		    : detail::ResultOwnershipBase<detail::ResultStorage<void, E>>(other)
		{
		}

		constexpr Result(Result &&other)
		    : detail::ResultOwnershipBase<detail::ResultStorage<void, E>>(
		          detail::move_ref(other))
		{
		}

		constexpr auto operator=(Result const &other) -> Result &
		{
			this->assign_copy(other);
			return *this;
		}

		constexpr auto operator=(Result &&other) -> Result &
		{
			this->assign_move(detail::move_ref(other));
			return *this;
		}

	private:
		constexpr Result(detail::ResultOkTag)
		    : detail::ResultOwnershipBase<detail::ResultStorage<void, E>>()
		{
			this->m_storage.construct_ok();
		}

		constexpr Result(detail::ResultErrTag, E error)
		    : detail::ResultOwnershipBase<detail::ResultStorage<void, E>>()
		{
			this->m_storage.construct_err(detail::move_ref(error));
		}
	};

	}
}
