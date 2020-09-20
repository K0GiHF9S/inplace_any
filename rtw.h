#pragma once

namespace lib
{
#ifdef _WIN32
#   ifdef _WIN64
using size_t = unsigned long long;
#   else
using size_t = unsigned long;
#   endif
#elif defined __GNUC__
using size_t = unsigned long;
#else
#   error not implemented
#endif

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
struct remove_reference {
    using type = T;
};

template <class T>
struct remove_reference<T&> {
    using type = T;
};

template <class T>
struct remove_reference<T&&> {
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
constexpr remove_reference_t<T>&& move(T&& value) noexcept {
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

template <class T, class U>
struct is_same
{
    static constexpr bool value = false;
};

template <class T>
struct is_same<T, T>
{
    static constexpr bool value = true;
};

template <bool test, class T, class U>
using conditional_t = typename conditional<test, T, U>::type;

namespace internal
{
struct bit64_tag {};
struct bit32_tag {};
using bit_size_tag = conditional_t<(sizeof(void*) == 8), bit64_tag, bit32_tag>;
}

template <class T, T Value>
struct integral_constant
{
    static constexpr T value = Value;
    using valueTpe = T;
    using type = integral_constant;

    constexpr operator valueTpe(void) const noexcept
    {
        return value;
    }

    constexpr valueTpe operator()(void) const noexcept
    {
        return value;
    }
};

template <bool Value>
using bool_constant = integral_constant<bool, Value>;

using true_type = bool_constant<true>;
using false_type = bool_constant<false>;

}