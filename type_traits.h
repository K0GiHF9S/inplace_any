#pragma once

namespace lib
{
#ifdef _WIN32
#ifdef _WIN64
using size_t = unsigned long long;
#else
using size_t = unsigned long;
#endif
#elif defined __GNUC__
using size_t = unsigned long;
#else
#error not implemented
#endif

using nullptr_t = decltype(nullptr);

template <bool, class = void>
struct enable_if
{};

template <class T>
struct enable_if<true, T>
{
    using type = T;
};

template <bool test, class T = void>
using enable_if_t = typename enable_if<test, T>::type;

template <bool test, class T = void>
struct disable_if : enable_if<!test, T>
{};

template <bool test, class T = void>
using disable_if_t = typename disable_if<test, T>::type;

template <class T>
struct remove_reference
{
    using type = T;
};

template <class T>
struct remove_reference<T&>
{
    using type = T;
};

template <class T>
struct remove_reference<T&&>
{
    using type = T;
};

template <class T>
using remove_reference_t = typename remove_reference<T>::type;

template <class T>
struct remove_const
{
    using type = T;
};

template <class T>
struct remove_const<const T>
{
    using type = T;
};

template <class T>
using remove_const_t = typename remove_const<T>::type;

template <class T>
struct remove_volatile
{
    using type = T;
};

template <class T>
struct remove_volatile<volatile T>
{
    using type = T;
};

template <class T>
using remove_volatile_t = typename remove_volatile<T>::type;

template <class T>
struct remove_cv
{
    using type = T;
};

template <class T>
struct remove_cv<const T>
{
    using type = T;
};

template <class T>
struct remove_cv<volatile T>
{
    using type = T;
};

template <class T>
struct remove_cv<const volatile T>
{
    using type = T;
};

template <class T>
using remove_cv_t = typename remove_cv<T>::type;

template <class T>
struct remove_cvref
{
    using type = remove_cv_t<remove_reference_t<T>>;
};

template <class T>
using remove_cvref_t = typename remove_cvref<T>::type;

template <class T>
struct remove_extent
{
    typedef T type;
};

template <class T>
struct remove_extent<T[]>
{
    typedef T type;
};

template <class T, size_t N>
struct remove_extent<T[N]>
{
    typedef T type;
};

template <class T>
using remove_extent_t = typename remove_extent<T>::type;

template <class T>
constexpr T&& forward(remove_reference_t<T>& value) noexcept
{
    return static_cast<T&&>(value);
}

template <class T>
constexpr T&& forward(remove_reference_t<T>&& value) noexcept
{
    return static_cast<T&&>(value);
}

template <class T>
constexpr remove_reference_t<T>&& move(T&& value) noexcept
{
    return static_cast<remove_reference_t<T>&&>(value);
}

template <bool, class T, class>
struct conditional
{
    using type = T;
};

template <class T, class U>
struct conditional<false, T, U>
{
    using type = U;
};

template <class T, T Value>
struct integral_constant
{
    static constexpr T value = Value;
    using valueTpe           = T;
    using type               = integral_constant;

    constexpr operator valueTpe(void) const noexcept { return value; }

    constexpr valueTpe operator()(void) const noexcept { return value; }
};

template <bool Value>
using bool_constant = integral_constant<bool, Value>;

using true_type  = bool_constant<true>;
using false_type = bool_constant<false>;

template <bool test, class T, class U>
using conditional_t = typename conditional<test, T, U>::type;

template <class...>
struct conjunction : true_type
{};
template <class T1>
struct conjunction<T1> : T1
{};
template <class T1, class... Ts>
struct conjunction<T1, Ts...> : conditional_t<T1::value, conjunction<Ts...>, T1>
{};

template <class...>
struct disjunction : false_type
{};
template <class T1>
struct disjunction<T1> : T1
{};
template <class T1, class... Ts>
struct disjunction<T1, Ts...> : conditional_t<T1::value, T1, disjunction<Ts...>>
{};

template <class T>
struct negation : bool_constant<!T::value>
{};

template <class...>
using void_t = void;

namespace internal
{
template <class T, class = void>
struct _add_lvalue_reference
{
    using type = T;
};

template <class T>
struct _add_lvalue_reference<T, void_t<T&>>
{
    using type = T&;
};

template <class T, class = void>
struct _add_rvalue_reference
{
    using type = T;
};

template <class T>
struct _add_rvalue_reference<T, void_t<T&>>
{
    using type = T&&;
};
}

template <class T>
struct add_lvalue_reference : internal::_add_lvalue_reference<T>
{};

template <class T>
using add_lvalue_reference_t = typename add_lvalue_reference<T>::type;

template <class T>
struct add_rvalue_reference : internal::_add_rvalue_reference<T>
{};

template <class T>
using add_rvalue_reference_t = typename add_rvalue_reference<T>::type;

template <class T>
add_rvalue_reference_t<T> declval() noexcept;

namespace internal
{
template <class T, class = void>
struct _add_pointer
{
    using type = T;
};

template <class T>
struct _add_pointer<T, void_t<remove_reference_t<T>*>>
{
    using type = remove_reference_t<T>*;
};
}

template <class T>
struct add_pointer : internal::_add_pointer<T>
{};

template <class T>
using add_pointer_t = typename add_pointer<T>::type;

template <class T>
struct add_const
{
    typedef T const type;
};

template <class T>
struct add_volatile
{
    typedef T volatile type;
};

template <class T>
using add_const_t = typename add_const<T>::type;

template <class T>
using add_volatile_t = typename add_volatile<T>::type;

template <class T>
struct add_cv
{
    using type = add_const_t<add_volatile_t<T>>;
};

template <class T>
using add_cv_t = typename add_cv<T>::type;

namespace internal
{
template <class, class T, class... Args>
struct _is_constructible : false_type
{};

template <class T, class... Args>
struct _is_constructible<void_t<decltype(T(declval<Args>()...))>, T, Args...> : true_type
{};
}

template <class T, class... Args>
using is_constructible = internal::_is_constructible<void, T, Args...>;

template <class T, class U>
struct is_same : false_type
{};

template <class T>
struct is_same<T, T> : true_type
{};

template <class T>
struct is_const : false_type
{};

template <class T>
struct is_const<const T> : true_type
{};

template <class T>
struct is_reference : false_type
{};

template <class T>
struct is_reference<T&> : true_type
{};

template <class T>
struct is_reference<T&&> : true_type
{};

template <class T>
struct is_function : negation<disjunction<is_const<const T>, is_reference<T>>>
{};

template <class T>
struct is_array : false_type
{};

template <class T>
struct is_array<T[]> : true_type
{};

template <class T, size_t N>
struct is_array<T[N]> : true_type
{};

template <class T>
struct is_void : is_same<remove_cv_t<T>, void>
{};

template <class T>
struct is_object : negation<disjunction<is_function<T>, is_reference<T>, is_void<T>>>
{};

template <class, class>
struct is_same_template : false_type
{};

template <template <class...> class T, class... A, class... B>
struct is_same_template<T<A...>, T<B...>> : true_type
{};

template <template <size_t...> class T, size_t... A, size_t... B>
struct is_same_template<T<A...>, T<B...>> : true_type
{};

template <template <class...> class, class>
struct is_template_of : false_type
{};

template <template <class...> class T, class... A>
struct is_template_of<T, T<A...>> : true_type
{};

template <class T>
struct decay
{
private:
    using U  = remove_reference_t<T>;
    using T1 = conditional_t<is_function<U>::value, add_pointer_t<U>, remove_cv_t<U>>;

public:
    using type = conditional_t<is_array<U>::value, remove_extent_t<U>*, T1>;
};

template <class T>
using decay_t = typename decay<T>::type;

template <class T, T... Index>
struct integer_sequence
{
    using value_type = T;
    static constexpr size_t size(void) noexcept { return sizeof...(Index); }
};

template <size_t... Index>
using index_sequence = integer_sequence<size_t, Index...>;

namespace internal
{
template <class T, T N, class = void>
struct make_integer_sequence_impl
{
    template <class>
    struct tmp;

    template <T... Prev>
    struct tmp<integer_sequence<T, Prev...>>
    {
        using type = integer_sequence<T, Prev..., N - 1>;
    };
    using type = typename tmp<typename make_integer_sequence_impl<T, N - 1>::type>::type;
};

template <class T, T N>
struct make_integer_sequence_impl<T, N, enable_if_t<N == 0>>
{
    using type = integer_sequence<T>;
};
}
template <class T, T N>
using make_integer_sequence = typename internal::make_integer_sequence_impl<T, N>::type;

template <size_t N>
using make_index_sequence = make_integer_sequence<size_t, N>;

namespace internal
{
union _max_align_t
{
    int         i;
    long long   ll;
    long double ld;
    void*       p;
    void (*fp)(void);
    struct dummy
    {};
    int dummy::*mp;
    void (dummy::*mfp)(void);
};
}

using max_align_t = internal::_max_align_t;

template <class First, class... Next>
class largest
{
private:
    using next_type = largest<Next...>;

public:
    static constexpr size_t SIZE  = sizeof(First) > next_type::SIZE ? sizeof(First) : next_type::SIZE;
    static constexpr size_t ALIGN = alignof(First) > next_type::ALIGN ? alignof(First) : next_type::ALIGN;
};

template <class First>
class largest<First>
{
public:
    static constexpr size_t SIZE  = sizeof(First);
    static constexpr size_t ALIGN = alignof(First);
};

}