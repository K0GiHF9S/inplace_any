#pragma once

#include "type_traits.h"

namespace lib
{
template <class... Ts>
class variant;

namespace internal
{
template <class T, class Variant>
struct _type_to_index;

template <class T, class First, class... Next>
struct _type_to_index<T, variant<First, Next...>> :
    integral_constant<size_t, _type_to_index<T, variant<Next...>>::value + 1>
{};

template <class T, class... Next>
struct _type_to_index<T, variant<T, Next...>> : integral_constant<size_t, 0>
{};

template <class T, class Variant>
struct _type_duple_count;

template <class T, class First, class... Next>
struct _type_duple_count<T, variant<First, Next...>> :
    integral_constant<size_t, _type_duple_count<T, variant<Next...>>::value>
{};

template <class T, class... Next>
struct _type_duple_count<T, variant<T, Next...>> :
    integral_constant<size_t, _type_duple_count<T, variant<Next...>>::value + 1>
{};

template <class T>
struct _type_duple_count<T, variant<>> : integral_constant<size_t, 0>
{};
}

template <class Variant>
struct variant_size;

template <class... Ts>
struct variant_size<variant<Ts...>> : integral_constant<size_t, sizeof...(Ts)>
{};

template <size_t Index, class Variants>
struct variant_alternative;

template <size_t Index, class First, class... Next>
struct variant_alternative<Index, variant<First, Next...>> : variant_alternative<Index - 1, variant<Next...>>
{};

template <class First, class... Next>
struct variant_alternative<0, variant<First, Next...>>
{
    using type = First;
};

template <size_t Index, class Variants>
using variant_alternative_t = typename variant_alternative<Index, Variants>::type;

template <size_t Index, class Variants>
struct variant_alternative<Index, const Variants> : add_const_t<variant_alternative_t<Index, remove_const_t<Variants>>>
{};

template <size_t Index, class Variants>
struct variant_alternative<Index, volatile Variants> :
    add_volatile_t<variant_alternative_t<Index, remove_volatile_t<Variants>>>
{};

template <size_t Index, class Variants>
struct variant_alternative<Index, const volatile Variants> :
    add_cv_t<variant_alternative_t<Index, remove_cv_t<Variants>>>
{};

namespace internal
{
template <class Visitor, class... Variants>
using _visit_result_t =
    decltype(lib::declval<Visitor>()(lib::declval<variant_alternative_t<0, remove_cvref_t<Variants>>>()...));
}

template <class Visitor, class... Variants>
internal::_visit_result_t<Visitor, Variants...> visit(Visitor&& vis, Variants&&... vars);

template <class... Ts>
class variant
{
    static_assert(!disjunction<is_array<Ts>...>::value, "Array cannot be used.");
    static_assert(conjunction<is_object<Ts>...>::value, "T params must be object.");

private:
    template <size_t Index, size_t N>
    struct copier
    {
        static void copy(const variant& src, variant& dst)
        {
            using type = variant_alternative_t<Index, variant>;
            if (src._current_id == Index)
            {
                new (dst._buffer) type(*static_cast<const type*>(src._get_buffer()));
            }
            else
            {
                copier<Index + 1, N>::copy(src, dst);
            }
        }
    };
    template <size_t N>
    struct copier<N, N>
    {
        static void copy(const variant& src, variant& dst) {}
    };

    template <size_t Index, size_t N>
    struct mover
    {
        static void move(variant& src, variant& dst)
        {
            using type = variant_alternative_t<Index, variant>;
            if (src._current_id == Index)
            {
                new (dst._buffer) type(::lib::move(*static_cast<type*>(src._get_buffer())));
            }
            else
            {
                mover<Index + 1, N>::move(src, dst);
            }
        }
    };
    template <size_t N>
    struct mover<N, N>
    {
        static void move(variant& src, variant& dst) {}
    };

    template <size_t Index, size_t N>
    struct destroyer
    {
        static void destroy(variant& target)
        {
            using type = variant_alternative_t<Index, variant>;
            if (target._current_id == Index)
            {
                static_cast<type*>(target._get_buffer())->~type();
            }
            else
            {
                destroyer<Index + 1, N>::destroy(target);
            }
        }
    };
    template <size_t N>
    struct destroyer<N, N>
    {
        static void destroy(variant& target) {}
    };

public:
    template <class T>
    using type_to_index = typename internal::_type_to_index<T, variant>;

private:
    alignas(Ts...) char _buffer[largest<Ts...>::SIZE]{};
    size_t _current_id;

public:
    variant(void) : _current_id(0)
    {
        using T = variant_alternative_t<0, variant>;
        static_assert(is_constructible<T>::value, "First T param is not constructible by default.");
        new (_buffer) T;
    }
    variant(const variant& rhs) : _current_id(rhs._current_id)
    {
        copier<0, variant_size<variant>::value>::copy(rhs, *this);
    }
    variant(variant&& rhs) : _current_id(rhs._current_id) { mover<0, variant_size<variant>::value>::move(rhs, *this); }

    template <class T, disable_if_t<disjunction<is_template_of<variant, decay_t<T>>>::value>* = nullptr>
    variant(T&& data) : _current_id(type_to_index<remove_cvref_t<T>>::value)
    {
        new (_buffer) remove_cvref_t<T>{forward<T>(data)};
    }

    variant& operator=(const variant& rhs)
    {
        destroy();
        _current_id = rhs._current_id;
        copier<0, variant_size<variant>::value>::copy(rhs, *this);
        return (*this);
    }
    variant& operator=(variant&& rhs)
    {
        destroy();
        _current_id = rhs._current_id;
        mover<0, variant_size<variant>::value>::move(rhs, *this);
        return (*this);
    }
    template <class T, disable_if_t<disjunction<is_template_of<variant, decay_t<T>>>::value>* = nullptr>
    variant& operator=(T&& data)
    {
        destroy();
        _current_id = type_to_index<remove_cvref_t<T>>::value;
        new (_buffer) remove_cvref_t<T>{forward<T>(data)};
        return (*this);
    }

    ~variant(void) { destroy(); }

    constexpr size_t index(void) const noexcept { return (_current_id); }

    void*       _get_buffer(void) { return (_buffer); }
    const void* _get_buffer(void) const { return (_buffer); }

private:
    void destroy(void) { destroyer<0, variant_size<variant>::value>::destroy(*this); }
};

template <size_t Index, class... Ts>
constexpr add_pointer_t<variant_alternative_t<Index, variant<Ts...>>> get_if(variant<Ts...>* v) noexcept
{
    using R = add_pointer_t<variant_alternative_t<Index, variant<Ts...>>>;
    static_assert(Index < sizeof...(Ts), "Index is out of bounds.");
    return ((v && v->index() == Index) ? static_cast<R>(v->_get_buffer()) : nullptr);
}

template <size_t Index, class... Ts>
constexpr add_pointer_t<const variant_alternative_t<Index, variant<Ts...>>> get_if(const variant<Ts...>* v) noexcept
{
    using R = add_pointer_t<const variant_alternative_t<Index, variant<Ts...>>>;
    static_assert(Index < sizeof...(Ts), "Index is out of bounds.");
    return ((v && v->index() == Index) ? static_cast<R>(v->_get_buffer()) : nullptr);
}

template <class T, class... Ts>
constexpr add_pointer_t<T> get_if(variant<Ts...>* v) noexcept
{
    using R = add_pointer_t<T>;
    static_assert(internal::_type_duple_count<T, variant<Ts...>>::value == 1, "variant tparam must have one T.");
    return ((v && v->index() == internal::_type_to_index<T, variant<Ts...>>::value) ? static_cast<R>(v->_get_buffer())
                                                                                    : nullptr);
}

template <class T, class... Ts>
constexpr add_pointer_t<const T> get_if(const variant<Ts...>* v) noexcept
{
    using R = add_pointer_t<const T>;
    static_assert(internal::_type_duple_count<T, variant<Ts...>>::value == 1, "variant tparam must have one T.");
    return ((v && v->index() == internal::_type_to_index<T, variant<Ts...>>::value) ? static_cast<R>(v->_get_buffer())
                                                                                    : nullptr);
}

template <size_t Index, class... Ts>
constexpr variant_alternative_t<Index, variant<Ts...>>& get(variant<Ts...>& v)
{
    return (*get_if<Index>(&v));
}

template <size_t Index, class... Ts>
constexpr variant_alternative_t<Index, variant<Ts...>>&& get(variant<Ts...>&& v)
{
    return (move(*get_if<Index>(&v)));
}

template <size_t Index, class... Ts>
constexpr const variant_alternative_t<Index, variant<Ts...>>& get(const variant<Ts...>& v)
{
    return (*get_if<Index>(&v));
}

template <size_t Index, class... Ts>
constexpr const variant_alternative_t<Index, variant<Ts...>>&& get(const variant<Ts...>&& v)
{
    return (move(*get_if<Index>(&v)));
}

template <class T, class... Ts>
constexpr T& get(variant<Ts...>& v)
{
    return (*get_if<T>(&v));
}

template <class T, class... Ts>
constexpr T&& get(variant<Ts...>&& v)
{
    return (move(*get_if<T>(&v)));
}

template <class T, class... Ts>
constexpr const T& get(const variant<Ts...>& v)
{
    return (*get_if<T>(&v));
}

template <class T, class... Ts>
constexpr const T&& get(const variant<Ts...>&& v)
{
    return (move(*get_if<T>(&v)));
}

namespace internal
{
template <size_t Index, size_t N, class = void>
struct _visit_impl
{
    template <class R, class Visitor, class Variants>
    static R _visit(Visitor&& vis, Variants&& vars)
    {
        static_assert(_type_duple_count<variant_alternative_t<Index, remove_cvref_t<Variants>>,
                                        remove_cvref_t<Variants>>::value == 1,
                      "variant tparam must have one T.");
        if (vars.index() == Index)
        {
            return (forward<Visitor>(vis)(get<Index>(forward<Variants>(vars))));
        }
        return (_visit_impl<Index + 1, N>::template _visit<R>(vis, vars));
    }
};

template <size_t Index, size_t N>
struct _visit_impl<Index, N, enable_if_t<Index == N - 1>>
{
    template <class R, class Visitor, class Variants>
    static R _visit(Visitor&& vis, Variants&& vars)
    {
        static_assert(_type_duple_count<variant_alternative_t<Index, remove_cvref_t<Variants>>,
                                        remove_cvref_t<Variants>>::value == 1,
                      "variant tparam must have one T.");
        return (forward<Visitor>(vis)(get<Index>(forward<Variants>(vars))));
    }
};
}

template <class Visitor, class... Variants>
inline internal::_visit_result_t<Visitor, Variants...> visit(Visitor&& vis, Variants&&... vars)
{
    using R = internal::_visit_result_t<Visitor, Variants...>;
    static_assert(conjunction<is_template_of<variant, remove_cvref_t<Variants>>...>::value, "Vars must be variant.");
    return internal::_visit_impl<0, variant_size<remove_cvref_t<Variants>>::value...>::template _visit<R>(
        forward<Visitor>(vis), forward<Variants>(vars)...);
}

template <class R, class Visitor, class... Variants>
inline R visit(Visitor&& vis, Variants&&... vars)
{
    static_assert(conjunction<is_template_of<variant, remove_cvref_t<Variants>>...>::value, "Vars must be variant.");
    return internal::_visit_impl<0, variant_size<remove_cvref_t<Variants>>::value...>::template _visit<R>(
        forward<Visitor>(vis), forward<Variants>(vars)...);
}
}
