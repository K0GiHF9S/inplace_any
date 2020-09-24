#pragma once

#include "type_traits.h"
#include "new.h"

namespace lib
{

namespace internal
{
enum class _any_operater
{
    Delete,
    Copy,
    Move,
};

template <class T>
struct _any_manager
{
    static void invoke(const void* src, void* dst, _any_operater ope)
    {
        auto* p = static_cast<const T*>(src);
        switch (ope)
        {
        case _any_operater::Delete:
            p->~T();
            break;
        case _any_operater::Copy:
            ::new (dst) T(*p);
            break;
        case _any_operater::Move:
            ::new (dst) T(move(*const_cast<T*>(p)));
            p->~T();
            break;
        default:
            break;
        }
    }
};

using _any_invoker_type = void (*)(const void* src, void* dst, _any_operater ope);

struct bit64_tag
{};
struct bit32_tag
{};
using bit_size_tag = conditional_t<(sizeof(void*) == 8), bit64_tag, bit32_tag>;

template <class Tag>
struct hash_param;

template <>
struct hash_param<bit64_tag>
{
    static constexpr size_t fnv_prime    = 1099511628211ull;
    static constexpr size_t offset_basis = 14695981039346656037ull;
};

template <>
struct hash_param<bit32_tag>
{
    static constexpr size_t fnv_prime    = 16777619ull;
    static constexpr size_t offset_basis = 2166136261ull;
};

using hash_param_t = hash_param<bit_size_tag>;

inline constexpr size_t calc_fnv1a_hash(const char* values, size_t index)
{
    return (((index ? calc_fnv1a_hash(values, index - 1) : hash_param_t::offset_basis) ^ values[index]) *
            hash_param_t::fnv_prime);
}
}

constexpr size_t operator"" _hash(const char* str, size_t length) { return internal::calc_fnv1a_hash(str, length - 1); }

template <class T>
struct in_place_type_t
{
    explicit in_place_type_t() = default;
};

template <class T>
struct in_place_type_v
{
    static constexpr in_place_type_t<T> value{};
};

namespace internal
{

class _any
{
private:
    void* const                 _buffer;
    internal::_any_invoker_type _invoker = nullptr;

public:
    bool has_value(void) const noexcept { return (_invoker); }

    void reset(void) noexcept
    {
        if (_invoker)
        {
            _invoker(_buffer, nullptr, internal::_any_operater::Delete);
            _invoker = nullptr;
        }
    }

    template <class T>
    T* _cast(void) const
    {
        return (_invoker == internal::_any_manager<decay_t<T>>::invoke ? static_cast<decay_t<T>*>(_buffer) : nullptr);
    }

protected:
    explicit _any(char* buffer) noexcept : _buffer(buffer) {}

    ~_any(void) noexcept { reset(); }

    void copy_data(const _any& rhs)
    {
        reset();
        if (rhs._invoker)
        {
            rhs._invoker(rhs._buffer, _buffer, internal::_any_operater::Copy);
            _invoker = rhs._invoker;
        }
    }

    void move_data(_any&& rhs)
    {
        reset();
        if (rhs._invoker)
        {
            rhs._invoker(rhs._buffer, _buffer, internal::_any_operater::Move);
            _invoker     = rhs._invoker;
            rhs._invoker = nullptr;
        }
    }

    template <class Decayed, class... Args>
    Decayed& _emplace(Args&&... args)
    {
        reset();
        _invoker = internal::_any_manager<Decayed>::invoke;
        new (_buffer) Decayed{forward<Args>(args)...};
        return (*static_cast<Decayed*>(_buffer));
    }
};
}

template <class T>
T* any_cast(internal::_any* target) noexcept
{
    return (target ? target->_cast<T>() : nullptr);
}

template <class T>
const T* any_cast(const internal::_any* target) noexcept
{
    return (target ? target->_cast<T>() : nullptr);
}

template <class T>
T any_cast(internal::_any& target)
{
    using U = remove_cvref_t<T>;
    static_assert(is_constructible<T, const U&>::value, "T is not constructible");
    return (*any_cast<U>(&target));
}

template <class T>
T any_cast(const internal::_any& target)
{
    using U = remove_cvref_t<T>;
    static_assert(is_constructible<T, U&>::value, "T is not constructible");
    return (*any_cast<U>(&target));
}

template <class T>
T any_cast(internal::_any&& target)
{
    using U = remove_cvref_t<T>;
    static_assert(is_constructible<T, U>::value, "T is not constructible");
    return (move(*any_cast<U>(&target)));
}

template <size_t SIZE, size_t ALIGN = alignof(max_align_t)>
class any : public internal::_any
{
private:
    alignas(ALIGN) char _buffer[SIZE]{};

public:
    constexpr any(void) noexcept : _any(_buffer) {}

    any(const any& rhs) : _any(_buffer) { copy_data(rhs); }

    any(any&& rhs) noexcept : _any(_buffer) { move_data(move(rhs)); }

    template <class T, disable_if_t<disjunction<is_same<any, decay_t<T>>,
                                                is_template_of<in_place_type_t, decay_t<T>>>::value>* = nullptr>
    explicit any(T&& data) : _any(_buffer)
    {
        static_assert(!is_same_template<any, decay_t<T>>::value, "size or align is different");
        static_assert(sizeof(decay_t<T>) <= SIZE, "Insufficient size");
        static_assert(ALIGN % alignof(decay_t<T>) == 0, "Alignment is incorrect");
        _emplace<decay_t<T>>(forward<T>(data));
    }

    template <class T, class... Args>
    explicit any(in_place_type_t<T>, Args&&... data) : _any(_buffer)
    {
        static_assert(!is_same_template<any, decay_t<T>>::value, "size or align is different");
        static_assert(sizeof(decay_t<T>) <= SIZE, "Insufficient size");
        static_assert(ALIGN % alignof(decay_t<T>) == 0, "Alignment is incorrect");
        _emplace<decay_t<T>>(forward<Args>(data)...);
    }

    any& operator=(const any& rhs)
    {
        if (this != &rhs)
        {
            copy_data(rhs);
        }
        return (*this);
    }

    any& operator=(any&& rhs) noexcept
    {
        if (this != &rhs)
        {
            move_data(move(rhs));
        }
        return (*this);
    }

    template <class T, disable_if_t<disjunction<is_same<any, decay_t<T>>,
                                                is_template_of<in_place_type_t, decay_t<T>>>::value>* = nullptr>
    any& operator=(T&& data)
    {
        static_assert(!is_same_template<any, decay_t<T>>::value, "size or align is different");
        static_assert(sizeof(decay_t<T>) <= SIZE, "Insufficient size");
        static_assert(ALIGN % alignof(decay_t<T>) == 0, "Alignment is incorrect");
        _emplace<decay_t<T>>(forward<T>(data));
        return (*this);
    }

    template <class T, class... Args>
    decay_t<T>& emplace(Args&&... args)
    {
        static_assert(sizeof(decay_t<T>) <= SIZE, "Insufficient size");
        static_assert(ALIGN % alignof(decay_t<T>) == 0, "Alignment is incorrect");
        return (_emplace<decay_t<T>>(forward<Args>(args)...));
    }

    void swap(any& rhs) noexcept
    {
        if (has_value() && rhs.has_value())
        {
            if (this != &rhs)
            {
                const any tmp{rhs};
                rhs   = *this;
                *this = tmp;
            }
        }
        else if (!has_value() && !rhs.has_value())
        {
            // NOP
        }
        else
        {
            any* const src = has_value() ? this : &rhs;
            any* const dst = has_value() ? &rhs : this;

            *dst = move(*src);
        }
    }
};

template <size_t SIZE, size_t ALIGN>
void swap(any<SIZE, ALIGN>& lhs, any<SIZE, ALIGN>& rhs) noexcept
{
    lhs.swap(rhs);
}

namespace internal
{

template <class>
class _function;

template <class R, class... Args>
class _function<R(Args...)>
{
protected:
    using func_type    = R (*)(_any&, Args&&...);
    func_type _derived = nullptr;

private:
    _any* const _pfunc;

public:
    _function(_any* pfunc, func_type derived) noexcept : _pfunc(pfunc), _derived(derived) {}

    _function& operator=(nullptr_t)
    {
        _pfunc->reset();
        _derived = nullptr;
        return (*this);
    }

    R operator()(Args&&... args) { return (_derived(*_pfunc, forward<Args>(args)...)); }

    explicit operator bool(void) const noexcept { return (_derived); }

protected:
    template <class T>
    static R _invoke(_any& func, Args&&... args)
    {
        return (any_cast<decay_t<T>>(func)(forward<Args>(args)...));
    }
};
}

template <class, size_t SIZE, size_t ALIGN = alignof(max_align_t)>
class function;

template <class R, class... Args, size_t SIZE, size_t ALIGN>
class function<R(Args...), SIZE, ALIGN> : public internal::_function<R(Args...)>
{
    using base = internal::_function<R(Args...)>;

private:
    using any = any<SIZE, ALIGN>;
    any _func;

public:
    function(void) noexcept : base(&_func, nullptr) {}
    function(nullptr_t) noexcept : base(&_func, nullptr) {}
    function(const function& rhs) : base(&_func, rhs._derived), _func(rhs._func) {}
    function(function&& rhs) : base(&_func, rhs._derived), _func(rhs._func) { rhs = nullptr; }
    template <class F, disable_if_t<is_same<function, remove_cvref_t<F>>::value>* = nullptr>
    function(F&& func) : base(&_func, &base::template _invoke<F>), _func(move(func))
    {}

    function& operator=(const function& rhs)
    {
        _func          = rhs._func;
        this->_derived = rhs._derived;
        return (*this);
    }
    function& operator=(function&& rhs)
    {
        *this = static_cast<const function&>(rhs);
        rhs   = nullptr;
        return (*this);
    }
    template <class F, disable_if_t<is_same<function, remove_cvref_t<F>>::value>* = nullptr>
    function& operator=(F&& func)
    {
        _func          = move(func);
        this->_derived = &base::template _invoke<F>;
        return (*this);
    }

    function& operator=(nullptr_t)
    {
        base::operator=(nullptr);
        return (*this);
    }

    R operator()(Args&&... args) { return (this->_derived(_func, forward<Args>(args)...)); }

    explicit operator bool(void) const noexcept { return (this->_derived); }

    void swap(function& rhs)
    {
        if (*this && rhs)
        {
            if (this != &rhs)
            {
                const function tmp{rhs};
                rhs   = *this;
                *this = tmp;
            }
        }
        else if (!*this && !rhs)
        {
            // NOP
        }
        else
        {
            function* const src = *this ? this : &rhs;
            function* const dst = *this ? &rhs : this;

            *dst = move(*src);
        }
    }
};

template <class R, class... Args, size_t SIZE, size_t ALIGN>
void swap(function<R(Args...), SIZE, ALIGN>& lhs, function<R(Args...), SIZE, ALIGN>& rhs) noexcept
{
    lhs.swap(rhs);
}

template <class... Args>
struct fitted_any
{
    static constexpr size_t SIZE  = largest<Args...>::SIZE;
    static constexpr size_t ALIGN = largest<Args...>::ALIGN;

    using type = any<SIZE, ALIGN>;
    template <class Func>
    using invoker_type = function<Func, SIZE, ALIGN>;
};

template <size_t SIZE, size_t ALIGN, class T, class... Args>
any<SIZE, ALIGN> make_any(Args&&... args)
{
    return (any<SIZE, ALIGN>(in_place_type_v<T>::value, forward<Args>(args)...));
}

template <class T, class... Args>
typename fitted_any<T>::type make_fitted_any(Args&&... args)
{
    return (typename fitted_any<T>::type(in_place_type_v<T>::value, forward<Args>(args)...));
}

}