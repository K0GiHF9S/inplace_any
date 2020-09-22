#include "any.h"
#include "variant.h"
#include <iostream>
#include <memory>
class B
{
public:
    long long            b{};
    long                 a{};
    std::unique_ptr<int> p{new int(1)};
    B(void) { std::cout << "create B" << std::endl; }
    B(const B&) { std::cout << "copy B" << std::endl; }
    B(B&&) { std::cout << "move B" << std::endl; }
    ~B(void) { std::cout << "delete B:" << b << std::endl; }
};

class A
{
public:
    long                 aa{};
    short                a[3]{};
    std::unique_ptr<int> p{new int(5)};
    A(void) { std::cout << "create A" << std::endl; }
    A(const A&) { std::cout << "copy A" << std::endl; }
    A(A&&) { std::cout << "move A" << std::endl; }
    ~A(void) { std::cout << "delete A" << std::endl; }
};
class C
{
public:
    C()         = default;
    C(const C&) = default;
    C(C&&)      = delete;
};

struct T
{
    int a;
    int b;
};

struct U
{
    int  a = 1;
    int  b = 2;
    void operator()() { std::cout << __func__ << ":a = " << a << std::endl; }
};

void func(void) { std::cout << __func__ << std::endl; }

class Test
{
public:
    template <class T>
    explicit Test(T&& value)
    {
        static_assert(lib::is_same_template<lib::any<4, 4>, lib::remove_reference_t<T>>::value, "");
    }
    template <class T>
    Test& operator=(T&& value)
    {
        static_assert(lib::is_same_template<lib::any<4, 4>, lib::remove_reference_t<T>>::value, "");
        return (*this);
    }
};

void test_any(void)
{
    auto _a = lib::internal::_any_manager<int>::invoke;
    // Test t{ any<8, 4>() };
    // any<8, 4> k;
    // Test t2{ k };
    // t2 = k;
    // const any<4, 4> y;
    // Test t3{ y };
    // t2 = y;
    std::cout << "1" << std::endl;
    using any = lib::fitted_any<A, B>::type;
    {
        any a;
        any b{A()};
        B   p;
        any c{p};
        a = B();
    }
    std::cout << "2" << std::endl;
    {
        const A   p;
        const any a{p};
        any       b;
        b = a;
        any c{a};
        lib::any_cast<A>(a);
        lib::any_cast<A>(any{p});
    }
    std::cout << "3" << std::endl;
    {
        any a{A()};
        any b;
        b = lib::move(a);
        any c{lib::move(b)};
    }
    std::cout << "4" << std::endl;
    {
        using any2 = lib::any<64, 8>;
        const any a{A()};
        // any2 b{a};
        // any2 d{lib::move(a)};
        // any2 e{any{A()}};
    }
    {
        std::cout << "hash:" << std::hash<std::string>{}("a") << std::endl;
        using lib::           operator""_hash;
        constexpr lib::size_t h = "a"_hash;
        std::cout << "hash:"
                  << "a"_hash << std::endl;
    }
    {
        any f;
        f.emplace<int>(4);
        f.emplace<A>();
        f.emplace<A>();
        any g{B()};
        std::cout << lib::any_cast<A>(&f) << std::endl;
        std::cout << lib::any_cast<B>(&g) << std::endl;
        std::cout << lib::any_cast<int>(&f) << std::endl;
        std::cout << lib::any_cast<A&>(f).a << std::endl;
        std::cout << lib::any_cast<B&>(g).b << std::endl;
        lib::any_cast<B&>(g).b = 4;
        std::cout << lib::any_cast<B&>(g).b << std::endl;
        // try
        // {
        //     std::cout << lib::any_cast<int&>(f) << std::endl;
        // }
        // catch (const lib::bad_cast &)
        // {
        //     std::cout << "int cast is illigal" << std::endl;
        // };
    }
    {
        int  p  = 9;
        auto a  = [&p] { std::cout << "p:" << p << std::endl; };
        using t = decltype(a);
        using F = lib::fitted_any<t>::invoker_type<void(void)>;
        F f{a};
        f();
    }
    {
        auto a  = [] {};
        using t = decltype(a);
        lib::any<8> b{a};
        std::cout << "no pointer:" << lib::any_cast<void (*)(void)>(&b) << std::endl;
        b.emplace<void (*)(void)>(a);
        std::cout << "has pointer:" << lib::any_cast<void (*)(void)>(&b) << std::endl;
    }
    {
        lib::fitted_any<T>::type a{lib::in_place_type_v<T>::value, 1, 2};
        std::cout << "a:" << lib::any_cast<T&>(a).a << std::endl;
        std::cout << "b:" << lib::any_cast<T&>(a).b << std::endl;
    }
    {
        auto a = lib::make_any<32, 8, T>(3, 4);
        std::cout << "a:" << lib::any_cast<T&>(a).a << std::endl;
        std::cout << "b:" << lib::any_cast<T&>(a).b << std::endl;
    }
    {
        U u;

        lib::fitted_any<T>::invoker_type<void(void)> a{u};
        a();
        func();
    }
    {
        auto a = lib::make_any<8, 8, T>(3, 4);
        auto b = lib::make_any<8, 8, int>(3);
        a.swap(b);
        std::cout << "a:" << lib::any_cast<T&>(b).a << "b:" << lib::any_cast<T&>(b).b << std::endl;
        std::cout << lib::any_cast<int&>(a) << std::endl;
        decltype(a) x;
        decltype(a) y;
        lib::swap(x, y);
        std::cout << std::boolalpha << x.has_value() << std::endl;
        std::cout << std::boolalpha << y.has_value() << std::endl;
        lib::swap(a, x);
        std::cout << std::boolalpha << a.has_value() << std::endl;
        std::cout << std::boolalpha << x.has_value() << std::endl;
        lib::swap(a, x);
        std::cout << std::boolalpha << a.has_value() << std::endl;
        std::cout << std::boolalpha << x.has_value() << std::endl;
    }
    {
        int  m   = 3;
        auto lam = [m](int n) {
            std::cout << n + m << std::endl;
            return (n * m);
        };
        lib::fitted_any<decltype(lam)>::invoker_type<int(int)> a{lam};
        std::cout << a(5) << std::endl;
        a = [m](int n) {
            std::cout << n - m << std::endl;
            return (n / m);
        };
        std::cout << a(5) << std::endl;
        lib::fitted_any<decltype(lam)>::invoker_type<int(int)> b{lam};
        a = lib::move(b);
        std::cout << a(5) << std::endl;
        lib::fitted_any<decltype(lam)>::invoker_type<int(int)> c{a};
        std::cout << c(5) << std::endl;
        lib::fitted_any<decltype(lam)>::invoker_type<int(int)> d{lib::move(c)};
        std::cout << d(5) << std::endl;
        a.swap(b);
        // std::cout << a(5) << std::endl;
        std::cout << b(5) << std::endl;
        a.swap(b);
        std::cout << a(5) << std::endl;
        // std::cout << b(5) << std::endl;
        b = [](int n) { return n; };
        lib::swap(a, b);
        std::cout << a(5) << std::endl;
        std::cout << b(5) << std::endl;
    }
    {
        C c;
        // lib::fitted_any<C>::type cc{c};
    }
}
using lib::variant;

struct visitor
{
    int operator()(int)
    {
        std::cout << "visited int" << std::endl;
        return (1);
    }

    int operator()(A&)
    {
        std::cout << "visited A" << std::endl;
        return (2);
    }

    template <class T>
    int operator()(T&&){
        std::cout << "visited" << std::endl;
        return (3);
    }
};

void test_variant(void)
{
    lib::variant<int, A, int> v0;
    lib::variant<int, A>      v1{1};
    lib::variant<int, A>      v2{A{}};
    const A                   a;
    lib::variant<int, A>      v3{a};
    const auto&               vv = v1;

    v3 = A{};
    v3 = 2;
    v0 = 2;

    std::cout << lib::get_if<0>(&v0) << std::endl;
    std::cout << lib::get_if<2>(&v0) << std::endl;
    // std::cout << lib::get_if<3>(&v0) << std::endl;
    std::cout << lib::get_if<A>(&v0) << std::endl;
    // std::cout << lib::get_if<B>(&v0) << std::endl;
    // std::cout << lib::get_if<int>(&v0) << std::endl;

    const auto* vp = &v0;
    std::cout << lib::get_if<0>(vp) << std::endl;
    std::cout << lib::get_if<2>(vp) << std::endl;
    // std::cout << lib::get_if<3>(vp) << std::endl;
    std::cout << lib::get_if<A>(vp) << std::endl;
    // std::cout << lib::get_if<B>(vp) << std::endl;
    // std::cout << lib::get_if<int>(vp) << std::endl;

    std::cout << ++lib::get<1>(v2).aa << std::endl;
    std::cout << ++lib::get<A>(v2).aa << std::endl;
    auto r = lib::get<1>(move(v2));
    std::cout << *lib::get<A>(move(v2)).p << std::endl;

    std::cout << lib::get<0>(*vp) << std::endl;

    lib::visit(visitor{}, v1);
    v1 = A{};
    lib::visit(visitor{}, v1);

    v2 = v1;
    v3 = lib::move(v1);

    // auto b = []{};
    // lib::variant<decltype(b)> v4;
}
int main()
{
    // std::visit();
    test_any();
    test_variant();
    return 0;
}
