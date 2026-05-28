#pragma once

#include <CommonLib/Platform.h>
#include <CommonLib/Utility.h>

namespace CL {

namespace detail {

[[noreturn]] inline auto bad_function_call() -> void
{
	panic("bad function call");
}

template<typename R, bool Noexcept, typename... Args> struct FunctionBase {
	struct VTable {
		void *(*copy)(void const *);
		void (*destroy)(void *);
		R (*call)(void *, Args...) noexcept(Noexcept);
	};

	void *m_object {};
	VTable const *m_vtable {};

	FunctionBase() = default;

	FunctionBase(FunctionBase const &other)
	{
		if (other.m_object) {
			m_object = other.m_vtable->copy(other.m_object);
			m_vtable = other.m_vtable;
		}
	}

	FunctionBase(FunctionBase &&other)
	    : m_object { other.m_object }
	    , m_vtable { other.m_vtable }
	{
		other.m_object = nullptr;
		other.m_vtable = nullptr;
	}

	~FunctionBase() { clear(); }

	auto operator=(FunctionBase const &other) -> FunctionBase &
	{
		FunctionBase tmp { other };
		swap(tmp);
		return *this;
	}

	auto operator=(FunctionBase &&other) noexcept -> FunctionBase &
	{
		clear();

		m_object = other.m_object;
		m_vtable = other.m_vtable;

		other.m_object = nullptr;
		other.m_vtable = nullptr;

		return *this;
	}

	auto operator=(decltype(nullptr)) noexcept -> FunctionBase &
	{
		clear();
		return *this;
	}

	template<typename F> FunctionBase(F function)
	{
		using Fn = F;

		static VTable const table {
			.copy = [](void const *object) -> void * {
			    return new Fn(*static_cast<Fn const *>(object));
			},
			.destroy = [](void *object) { delete static_cast<Fn *>(object); },
			.call = [](void *object, Args... args) noexcept(Noexcept) -> R {
			    if constexpr (__is_void(R)) {
				    (*static_cast<Fn *>(object))(CL::forward<Args>(args)...);
			    } else {
				    return (*static_cast<Fn *>(object))(
				        CL::forward<Args>(args)...);
			    }
			},
		};

		m_object = new Fn(CL::move(function));
		m_vtable = &table;
	}

	auto clear() noexcept -> void
	{
		if (m_object)
			m_vtable->destroy(m_object);

		m_object = nullptr;
		m_vtable = nullptr;
	}

	auto swap(FunctionBase &other) noexcept -> void
	{
		CL::swap(m_object, other.m_object);
		CL::swap(m_vtable, other.m_vtable);
	}

	explicit operator bool() const { return m_object != nullptr; }

	auto operator==(decltype(nullptr)) const -> bool
	{
		return m_object == nullptr;
	}

	auto operator!=(decltype(nullptr)) const -> bool
	{
		return m_object != nullptr;
	}
};

template<typename R, bool Noexcept, typename... Args> struct ConstFunctionBase {
	struct VTable {
		void *(*copy)(void const *);
		void (*destroy)(void *);
		R (*call)(void const *, Args...) noexcept(Noexcept);
	};

	void *m_object {};
	VTable const *m_vtable {};

	ConstFunctionBase() = default;

	ConstFunctionBase(ConstFunctionBase const &other)
	{
		if (other.m_object) {
			m_object = other.m_vtable->copy(other.m_object);
			m_vtable = other.m_vtable;
		}
	}

	ConstFunctionBase(ConstFunctionBase &&other)
	    : m_object { other.m_object }
	    , m_vtable { other.m_vtable }
	{
		other.m_object = nullptr;
		other.m_vtable = nullptr;
	}

	~ConstFunctionBase() { clear(); }

	auto operator=(ConstFunctionBase const &other) -> ConstFunctionBase &
	{
		ConstFunctionBase tmp { other };
		swap(tmp);
		return *this;
	}

	auto operator=(ConstFunctionBase &&other) noexcept -> ConstFunctionBase &
	{
		clear();

		m_object = other.m_object;
		m_vtable = other.m_vtable;

		other.m_object = nullptr;
		other.m_vtable = nullptr;

		return *this;
	}

	auto operator=(decltype(nullptr)) noexcept -> ConstFunctionBase &
	{
		clear();
		return *this;
	}

	template<typename F> ConstFunctionBase(F function)
	{
		using Fn = F;

		static VTable const table {
			.copy = [](void const *object) -> void * {
			    return new Fn(*static_cast<Fn const *>(object));
			},
			.destroy = [](void *object) { delete static_cast<Fn *>(object); },
			.call
			= [](void const *object, Args... args) noexcept(Noexcept) -> R {
			    if constexpr (__is_void(R)) {
				    (*static_cast<Fn const *>(object))(
				        CL::forward<Args>(args)...);
			    } else {
				    return (*static_cast<Fn const *>(object))(
				        CL::forward<Args>(args)...);
			    }
			},
		};

		m_object = new Fn(CL::move(function));
		m_vtable = &table;
	}

	auto clear() noexcept -> void
	{
		if (m_object)
			m_vtable->destroy(m_object);

		m_object = nullptr;
		m_vtable = nullptr;
	}

	auto swap(ConstFunctionBase &other) noexcept -> void
	{
		CL::swap(m_object, other.m_object);
		CL::swap(m_vtable, other.m_vtable);
	}

	explicit operator bool() const { return m_object != nullptr; }

	auto operator==(decltype(nullptr)) const -> bool
	{
		return m_object == nullptr;
	}

	auto operator!=(decltype(nullptr)) const -> bool
	{
		return m_object != nullptr;
	}
};

}

template<typename...> struct Function;

template<typename R, typename... Args>
struct Function<R(Args...)> : detail::FunctionBase<R, false, Args...> {
	using Base = detail::FunctionBase<R, false, Args...>;
	using Base::Base;

	auto operator()(Args... args) -> R
	{
		if (!this->m_object)
			detail::bad_function_call();

		return this->m_vtable->call(this->m_object, CL::forward<Args>(args)...);
	}

	auto swap(Function &other) noexcept -> void { Base::swap(other); }
};

template<typename R, typename... Args>
struct Function<R(Args...) noexcept> : detail::FunctionBase<R, true, Args...> {
	using Base = detail::FunctionBase<R, true, Args...>;
	using Base::Base;

	auto operator()(Args... args) noexcept -> R
	{
		if (!this->m_object)
			detail::bad_function_call();

		return this->m_vtable->call(this->m_object, CL::forward<Args>(args)...);
	}

	auto swap(Function &other) noexcept -> void { Base::swap(other); }
};

template<typename R, typename... Args>
struct Function<R(Args...) const>
    : detail::ConstFunctionBase<R, false, Args...> {
	using Base = detail::ConstFunctionBase<R, false, Args...>;
	using Base::Base;

	auto operator()(Args... args) const -> R
	{
		if (!this->m_object)
			detail::bad_function_call();

		return this->m_vtable->call(this->m_object, CL::forward<Args>(args)...);
	}

	auto swap(Function &other) noexcept -> void { Base::swap(other); }
};

template<typename R, typename... Args>
struct Function<R(Args...) const noexcept>
    : detail::ConstFunctionBase<R, true, Args...> {
	using Base = detail::ConstFunctionBase<R, true, Args...>;
	using Base::Base;

	auto operator()(Args... args) const noexcept -> R
	{
		if (!this->m_object)
			detail::bad_function_call();

		return this->m_vtable->call(this->m_object, CL::forward<Args>(args)...);
	}

	auto swap(Function &other) noexcept -> void { Base::swap(other); }
};

#define CL_FUNCTION_REFQUAL(QUAL) \
	template<typename R, typename... Args> \
	struct Function<R(Args...) QUAL> : Function<R(Args...)> { \
		using Base = Function<R(Args...)>; \
		using Base::Base; \
		auto operator()(Args... args) QUAL->R \
		{ \
			return Base::operator()(CL::forward<Args>(args)...); \
		} \
	};

#define CL_FUNCTION_REFQUAL_NOEXCEPT(QUAL) \
	template<typename R, typename... Args> \
	struct Function<R(Args...) QUAL noexcept> \
	    : Function<R(Args...) noexcept> { \
		using Base = Function<R(Args...) noexcept>; \
		using Base::Base; \
		auto operator()(Args... args) QUAL noexcept -> R \
		{ \
			return Base::operator()(CL::forward<Args>(args)...); \
		} \
	};

CL_FUNCTION_REFQUAL(&)
CL_FUNCTION_REFQUAL(&&)
CL_FUNCTION_REFQUAL_NOEXCEPT(&)
CL_FUNCTION_REFQUAL_NOEXCEPT(&&)

#undef CL_FUNCTION_REFQUAL
#undef CL_FUNCTION_REFQUAL_NOEXCEPT

template<typename R, typename... Args>
struct Function<R(Args...) const &> : Function<R(Args...) const> {
	using Base = Function<R(Args...) const>;
	using Base::Base;
};

template<typename R, typename... Args>
struct Function<R(Args...) const &&> : Function<R(Args...) const> {
	using Base = Function<R(Args...) const>;
	using Base::Base;
};

template<typename R, typename... Args>
struct Function<R(Args...) const & noexcept>
    : Function<R(Args...) const noexcept> {
	using Base = Function<R(Args...) const noexcept>;
	using Base::Base;
};

template<typename R, typename... Args>
struct Function<R(Args...) const && noexcept>
    : Function<R(Args...) const noexcept> {
	using Base = Function<R(Args...) const noexcept>;
	using Base::Base;
};

}
