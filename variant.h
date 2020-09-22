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
using _visit_result_t = decltype(declval<Visitor>()(declval<variant_alternative_t<0, remove_cvref_t<Variants>>>()...));

struct _visit_copier
{
    void* const dst;
    template <class T>
    void operator()(const T& src) const
    {
        new (dst) decay_t<T>(src);
    }
};
struct _visit_mover
{
    void* const dst;
    template <class T>
    void operator()(T&& src) const
    {
        new (dst) decay_t<T>(move(src));
    }
};
struct _visit_destructor
{
    template <class T>
    void operator()(T&& dst) const
    {
        using Decayed = decay_t<T>;
        dst.~Decayed();
    }
};
constexpr _visit_destructor _visit_destructor_v;
}

template <class Visitor, class... Variants>
internal::_visit_result_t<Visitor, Variants...> visit(Visitor&& vis, Variants&&... vars);

template <class... Ts>
class variant
{
    static_assert(!disjunction<is_array<Ts>...>::value, "Array cannot be used.");
    static_assert(conjunction<is_object<Ts>...>::value, "T params must be object.");

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
        copy(rhs);
    }
    variant(variant&& rhs) : _current_id(rhs._current_id) { move(::lib::move(rhs)); }

    template <class T, disable_if_t<disjunction<is_template_of<variant, decay_t<T>>>::value>* = nullptr>
    variant(T&& data) : _current_id(type_to_index<remove_cvref_t<T>>::value)
    {
        new (_buffer) remove_cvref_t<T>{forward<T>(data)};
    }

    variant& operator=(const variant& rhs)
    {
        destroy();
        _current_id = rhs._current_id;
        copy(rhs);
        return (*this);
    }
    variant& operator=(variant&& rhs)
    {
        destroy();
        _current_id = rhs._current_id;
        move(::lib::move(rhs));
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
    void copy(const variant& src)
    {
        visit(internal::_visit_copier{_buffer}, src);
    }
    void move(variant&& src)
    {
        visit(internal::_visit_mover{_buffer}, src);
    }
    void destroy(void) { visit(internal::_visit_destructor_v, *this); }
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
template <class R, class Visitor, class Variants, size_t Index>
static R _visit_vtable(Visitor&& vis, Variants&& vars)
{
    return (forward<Visitor>(vis)(get<Index>(forward<Variants>(vars))));
}

template <class R, class Visitor, class Variants, size_t... Indices>
static R _visit_impl(Visitor&& vis, Variants&& vars, index_sequence<Indices...>)
{
    constexpr R (*vtable[])(Visitor&&, Variants &&) = {_visit_vtable<R, Visitor, Variants, Indices>...};
    return (vtable[vars.index()](forward<Visitor>(vis), forward<Variants>(vars)));
}
}

template <class Visitor, class... Variants>
internal::_visit_result_t<Visitor, Variants...> visit(Visitor&& vis, Variants&&... vars)
{
    using R = internal::_visit_result_t<Visitor, Variants...>;
    return (internal::_visit_impl<R>(forward<Visitor>(vis), forward<Variants>(vars)...,
                              make_index_sequence<variant_size<remove_cvref_t<Variants>>::value>{}...));
}

template <class R, class Visitor, class... Variants>
R visit(Visitor&& vis, Variants&&... vars)
{
    return (internal::_visit_impl<R>(forward<Visitor>(vis), forward<Variants>(vars)...,
                              make_index_sequence<variant_size<remove_cvref_t<Variants>>::value>{}...));
}
}
