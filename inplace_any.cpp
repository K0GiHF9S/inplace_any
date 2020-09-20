#include "inplace_any.h"
#include <iostream>

class B 
{
public:
	long long b{};
	long a{};
	B(void)
	{
		std::cout << "create B" << std::endl;
	}
	B(const B&)
	{
		std::cout << "copy B" << std::endl;
	}
	B(B&&)
	{
		std::cout << "move B" << std::endl;
	}
	~B(void)
	{
		std::cout << "delete B:" << b << std::endl;
	}
};

class A
{
public:
	long aa{};
	short a[3]{};
	A(void)
	{
		std::cout << "create A" << std::endl;
	}
	A(const A&)
	{
		std::cout << "copy A" << std::endl;
	}
	A(A&&)
	{
		std::cout << "move A" << std::endl;
	}
	~A(void)
	{
		std::cout << "delete A" << std::endl;
	}
};

template <>
constexpr size_t lib::define_typeid<A>(void)
{
	return ("A"_hash);
}

template <>
constexpr size_t lib::define_typeid<B>(void)
{
	return ("B"_hash);
}

template <>
constexpr size_t lib::define_typeid<int>(void)
{
	return ("int"_hash);
}

class Test
{
public:
	template <class T>
	explicit Test(T&& value)
	{
		static_assert(lib::is_same_template<lib::inplace_any<4,4>, lib::remove_reference_t<T>>::value, "");
	}
	template <class T>
	Test& operator=(T&& value)
	{
		static_assert(lib::is_same_template<lib::inplace_any<4, 4>, lib::remove_reference_t<T>>::value, "");
		return (*this);
	}
};

int main()
{
	//Test t{ inplace_any<8, 4>() };
	//inplace_any<8, 4> k;
	//Test t2{ k };
	//t2 = k;
	//const inplace_any<4, 4> y;
	//Test t3{ y };
	//t2 = y;
	std::cout << "1" << std::endl;
	using any = lib::fitted_inplace_any<A, B>::type;
	{
		any a;
		std::cout << "expect move obj" << std::endl;
		any b{ A() };
		B p;
		std::cout << "expect copy obj" << std::endl;
		any c{ p };
		std::cout << "expect move obj" << std::endl;
		a = B();
	}
	std::cout << "2" << std::endl;
	{
		const A p;
		std::cout << "expect copy obj" << std::endl;
		const any a{ p };
		any b;
		std::cout << "expect copy tmp" << std::endl;
		b = a;
		std::cout << "expect copy tmp" << std::endl;
		any c{ a };
	}
	std::cout << "3" << std::endl;
	{
		std::cout << "expect move obj" << std::endl;
		any a{ A() };
		any b;
		std::cout << "expect move tmp" << std::endl;
		b = std::move(a);
		std::cout << "expect move tmp" << std::endl;
		any c{ std::move(b) };
	}
	std::cout << "4" << std::endl;
	{
		using any2 = lib::inplace_any<28, 8>;
		std::cout << "expect move obj" << std::endl;
		const any a{ A() };
		std::cout << "expect copy tmp" << std::endl;
		any2 b{ a };
		const any c;
		std::cout << "expect copy tmp" << std::endl;
		b = c;
		std::cout << "expect copy tmp" << std::endl;
		b = std::move(c);
		std::cout << "expect copy tmp" << std::endl;
		any2 d{ std::move(a) };
		std::cout << "expect move tmp" << std::endl;
		any2 e{ any{A()} };
		std::cout << "expect move tmp" << std::endl;
		b = std::move(e);
	}
	{
		std::cout << "hash:" << std::hash<std::string>{}("a") << std::endl;
		using lib::operator""_hash;
		constexpr lib::size_t h = "a"_hash;
		std::cout << "hash:" << "a"_hash << std::endl;
	}
	{
		any f;
		f.emplace<int>(4);
		f.emplace<A>();
		f.emplace<A>();
		any g{B()};
		std::cout << f.cast_adr<A>() << std::endl;
		std::cout << g.cast_adr<B>() << std::endl;
		std::cout << f.cast_adr<int>() << std::endl;
		std::cout << f.cast_ref<A>().a << std::endl;
		std::cout << g.cast_ref<B>().b << std::endl;
		g.cast_ref<B>().b = 4;
		std::cout << g.cast_ref<B>().b << std::endl;
		try
		{
			std::cout << f.cast_ref<int>() << std::endl;
		}
		catch (const lib::bad_cast&)
		{
			std::cout << "int cast is illigal" << std::endl;
		};
	}
	return 0;
}
