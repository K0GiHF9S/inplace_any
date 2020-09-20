#pragma once

#include "rtw.h"

namespace lib
{

namespace internal
{
struct iholder
{
	virtual ~iholder(void) = default;
	virtual void copy(void* buffer) const = 0;
	virtual void move(void* buffer) = 0;
};

template <class T>
struct holder : public iholder
{
	T held;

	template <class... Args>
	explicit holder(Args&&... args) : held{ forward<Args>(args)... }
	{}
	void copy(void* buffer) const override
	{
		new(buffer) holder{ held };
	}
	void move(void* buffer) override
	{
		new(buffer) holder{ ::lib::move(held) };
	}
};

template <class Tag>
struct hash_param;

template <>
struct hash_param<internal::bit64_tag>
{
	static constexpr size_t fnv_prime = 1099511628211ull;
	static constexpr size_t offset_basis = 14695981039346656037ull;
};

template <>
struct hash_param<internal::bit32_tag>
{
	static constexpr size_t fnv_prime = 16777619ull;
	static constexpr size_t offset_basis = 2166136261ull;
};

using hash_param_t = hash_param<internal::bit_size_tag>;

inline constexpr size_t calc_fnv1a_hash(const char* values, size_t index)
{
	return (((index ? calc_fnv1a_hash(values, index - 1) : hash_param_t::offset_basis) ^ values[index]) * hash_param_t::fnv_prime);
}
}

constexpr size_t operator"" _hash(const char* str, size_t length)
{
	return internal::calc_fnv1a_hash(str, length - 1);
}

template <class, class>
struct is_same_template : false_type {};

template <template <class...> class T, class... A, class... B>
struct is_same_template<T<A...>, T<B...>> : true_type {};

template <template <size_t...> class T, size_t... A, size_t... B>
struct is_same_template<T<A...>, T<B...>> : true_type {};

template <class T>
struct typeid_tag {};

template <class T>
constexpr size_t define_typeid(void);

template <class T>
constexpr size_t get_typeid(void)
{
	return (define_typeid<remove_const_t<remove_reference_t<T>>>());
}

struct bad_cast {};

template <size_t SIZE, size_t ALIGN>
class inplace_any
{
	template<size_t, size_t>
	friend class inplace_any;
private:
	size_t _typeid{};
	alignas(ALIGN) char _buffer[SIZE]{};
	bool _valid;

public:
	inplace_any(void) noexcept : _valid(false)
	{}

	inplace_any(const inplace_any& rhs) : _typeid(rhs._typeid), _valid(rhs._valid)
	{
		if (_valid)
		{
			new_data(rhs);
		}
	}

	inplace_any(inplace_any&& rhs) noexcept : _typeid(rhs._typeid), _valid(rhs._valid)
	{
		if (_valid)
		{
			new_data(move(rhs));
		}
	}

	template <size_t RHS_SIZE, size_t RHS_ALIGN>
	inplace_any(const inplace_any<RHS_SIZE, RHS_ALIGN>& rhs) : _typeid(rhs._typeid), _valid(rhs._valid)
	{
		if (_valid)
		{
			new_data(rhs);
		}
	}

	template <size_t RHS_SIZE, size_t RHS_ALIGN>
	inplace_any(inplace_any<RHS_SIZE, RHS_ALIGN>&& rhs) : _typeid(rhs._typeid), _valid(rhs._valid)
	{
		if (_valid)
		{
			new_data(move(rhs));
		}
	}

	template <class T,
		disable_if_t<is_same_template<inplace_any, remove_const_t<remove_reference_t<T>>>::value>* = nullptr>
		explicit inplace_any(T&& data) : _typeid(get_typeid<T>()), _valid(true)
	{
		new_data(forward<T>(data));
	}

	inplace_any& operator=(const inplace_any& rhs)
	{
		if (this != &rhs)
		{
			delete_data();
			_typeid = rhs._typeid;
			_valid = rhs._valid;
			if (_valid)
			{
				new_data(rhs);
			}
		}
		return (*this);
	}

	inplace_any& operator=(inplace_any&& rhs) noexcept
	{
		if (this != &rhs)
		{
			delete_data();
			_typeid = rhs._typeid;
			_valid = rhs._valid;
			if (_valid)
			{
				new_data(move(rhs));
			}
		}
		return (*this);
	}

	template <size_t RHS_SIZE, size_t RHS_ALIGN>
	inplace_any& operator=(const inplace_any<RHS_SIZE, RHS_ALIGN>& rhs)
	{
		delete_data();
		_typeid = rhs._typeid;
		_valid = rhs._valid;
		if (_valid)
		{
			new_data(rhs);
		}
		return (*this);
	}

	template <size_t RHS_SIZE, size_t RHS_ALIGN>
	inplace_any& operator=(inplace_any<RHS_SIZE, RHS_ALIGN>&& rhs)
	{
		delete_data();
		_typeid = rhs._typeid;
		_valid = rhs._valid;
		if (_valid)
		{
			new_data(move(rhs));
		}
		return (*this);
	}

	template <class T,
		disable_if_t<is_same_template<inplace_any, remove_const_t<remove_reference_t<T>>>::value>* = nullptr>
		inplace_any& operator=(T&& data)
	{
		delete_data();
		_typeid = get_typeid<T>();
		_valid = true;
		new_data(forward<T>(data));
		return (*this);
	}

	~inplace_any(void) noexcept
	{
		delete_data();
	}

	template <class T, class... Args>
	void emplace(Args&&... args)
	{
		using derived_holder = internal::holder<remove_reference_t<T>>;
		static_assert(sizeof(derived_holder) <= SIZE, "Insufficient size");
		static_assert(ALIGN % alignof(derived_holder) == 0, "Alignment is incorrect");
		delete_data();
		_typeid = get_typeid<T>();
		_valid = true;
		new(_buffer) derived_holder{ forward<Args>(args)... };
	}

	bool is_valid(void) const noexcept
	{
		return (_valid);
	}

	size_t define_typeid(void) const noexcept
	{
		return (_typeid);
	}

	template <class T>
	T* cast_adr(void)
	{
		return (get_typeid<T>() == _typeid
			? &static_cast<internal::holder<T>*>(reinterpret_cast<internal::iholder*>(_buffer))->held
			: nullptr
			);
	}

	template <class T>
	const T* cast_adr(void) const
	{
		return (const_cast<inplace_any*>(this)->cast_adr<T>());
	}

	template <class T>
	T& cast_ref(void)
	{
		T* adr = cast_adr<T>();
		if (!adr)
		{
			throw bad_cast{};
		}
		return (*adr);
	}

	template <class T>
	const T& cast_ref(void) const
	{
		const T* adr = cast_adr<T>();
		if (!adr)
		{
			throw bad_cast{};
		}
		return (*adr);
	}

private:
	template <class T,
		disable_if_t<is_same_template<inplace_any, remove_const_t<remove_reference_t<T>>>::value>* = nullptr>
		void new_data(T&& data)
	{
		using derived_holder = internal::holder<remove_reference_t<T>>;
		static_assert(sizeof(derived_holder) <= SIZE, "Insufficient size");
		static_assert(ALIGN % alignof(derived_holder) == 0, "Alignment is incorrect");
		new(_buffer) derived_holder{ forward<T>(data) };
	}

	template <size_t RHS_SIZE, size_t RHS_ALIGN>
	void new_data(const inplace_any<RHS_SIZE, RHS_ALIGN>& rhs)
	{
		static_assert(RHS_SIZE <= SIZE, "Insufficient size");
		static_assert(RHS_ALIGN <= ALIGN, "Insufficient alignment");
		reinterpret_cast<const internal::iholder*>(rhs._buffer)->copy(_buffer);
	}

	template <size_t RHS_SIZE, size_t RHS_ALIGN>
	void new_data(inplace_any<RHS_SIZE, RHS_ALIGN>&& rhs)
	{
		static_assert(RHS_SIZE <= SIZE, "Insufficient size");
		static_assert(RHS_ALIGN <= ALIGN, "Insufficient alignment");
		reinterpret_cast<internal::iholder*>(rhs._buffer)->move(_buffer);
		rhs._valid = false;
	}

	void delete_data(void)
	{
		if (_valid)
		{
			reinterpret_cast<internal::iholder*>(_buffer)->~iholder();
			_valid = false;
		}
	}
};

template <class First, class... Next>
class largest
{
private:
	using next_type = typename largest<Next...>::type;

public:
	using type = conditional_t<sizeof(First) >= sizeof(next_type), First, next_type>;
	static constexpr size_t SIZE = sizeof(type);
};

template <class First>
class largest<First>
{
public:
	using type = First;
	static constexpr size_t SIZE = sizeof(type);
};

template <class First, class... Next>
class largest_align
{
private:
	using next_type = typename largest_align<Next...>::type;

public:
	using type = conditional_t<alignof(First) >= alignof(next_type), First, next_type>;
	static constexpr size_t ALIGN = alignof(type);
};

template <class First>
class largest_align<First>
{
public:
	using type = First;
	static constexpr size_t ALIGN = alignof(type);
};

template <class... Args>
struct fitted_inplace_any
{
	static constexpr size_t SIZE = largest<internal::holder<Args>...>::SIZE;
	static constexpr size_t ALIGN = largest_align<internal::holder<Args>...>::ALIGN;
	using type = inplace_any<SIZE, ALIGN>;
};
}