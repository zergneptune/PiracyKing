#include <type_traits>
#include <fstream>
#include <sstream>
#include <cstdint>
#include "practice.hpp"
/*-----------------------*/

#define HAS_MEMBER(member)\
template<typename T, typename... Args>struct has_member_##member\
{\
	private:\
		template<typename U> static auto Check(int) -> decltype(std::declval<U>().member(std::declval<Args>()...), std::true_type());\
		template<typename U> static std::false_type Check(...);\
	public:\
		enum{value = std::is_same<decltype(Check<T>(0)), std::true_type>::value};\
};


template<typename T, typename... Args>
struct has_member_foo11
{
private:
	template<typename U>
	static auto check(int) ->
	decltype(std::declval<U>().foo(std::declval<Args>()...), std::true_type());

	template<typename U>
	static std::false_type check(...);

public:
	enum{
		value = std::is_same<decltype(check<T>(0)), std::true_type>::value
	};
};

struct MyStruct
{
    string foo() { return ""; }
	int foo(int i) { return i; }
};

template <unsigned n>
struct factorial : std::integral_constant<int,n * factorial<n-1>::value> {};

template <>
struct factorial<0> : std::integral_constant<int,1> {};


typedef int integer_type;
struct A { int x,y; };
struct B { int x,y; };
typedef A C;

/////////////////////////////////////////////////////////////
template<typename... Args>
void read_line(std::string& buf, Args&&... arg)
{
	std::istringstream instr(buf);
	int arr[] = { ((instr >> arg), 0)... };
	int arr_2[] = { ((std::cout << arg << '\t'), 0)... };
	std::cout << std::endl;
}

template<typename... Args>
void read_file(std::ifstream& infile)
{
	string buf;
	if(infile.is_open())
	{
		while(getline(infile, buf))
		{
			read_line(buf, Args()...);
		}
	}
}

template<typename First, typename... Rest>
void write_line(std::ofstream& outfile, First&& first, Rest&&... rest)
{
	outfile << first << '\t';
	write_line(outfile, rest...);
}

template<typename T>
void write_line(std::ofstream& outfile, T&& arg)
{
	outfile << arg << '\n';
}

class MyClass
{
public:
	MyClass(){}
	MyClass(int a, double b, std::string c): m_a(a), m_b(b), m_c(c){}
	~MyClass(){}

	friend std::istream& operator >> (std::istream& in, MyClass& arg)
	{
		in >> arg.m_a >> arg.m_b >> arg.m_c;
		return in;
	}

	friend std::ostream& operator << (std::ostream& out, MyClass& arg)
	{
		out << arg.m_a << '\t' << arg.m_b << '\t' << arg.m_c;
		return out;
	}

private:
	int m_a;
	double m_b;
	std::string m_c;
};

/*
** Union of all Lua values
*/
typedef union {
  void *gc;
  void *p;
  double n;
  int b;
} Value;


/*
** Tagged Values
*/

#define TValuefields	Value value; int tt

typedef struct lua_TValue {
  TValuefields;
} TValue;

typedef union TKey {
  struct {
    TValuefields; /* 16B */
    struct Node *next;  /* 8B */
  } nk;
  TValue tvk; /* 16 */
} TKey;

typedef struct Node {
  TValue i_val; /* 16 */
  TKey i_key; /* 24 */
} Node;

typedef struct tt
{
	char* p;	
}tt;

int main(int argc, char const *argv[])
{
	/*
	cout<< has_member_foo11<MyStruct>::value << endl;
	cout<< has_member_foo11<MyStruct, int>::value << endl;
	cout<< has_member_foo11<MyStruct, double>::value << endl;
	cout << factorial<5>::value << endl;  // constexpr (no calculations on runtime)
	static_assert(has_member_foo11<MyStruct>::value, "MyStruct has foo");
	static_assert(has_member_foo11<MyStruct, int>::value, "MyStruct has foo");

	cout << std::boolalpha;
	cout << "is_same:" << std::endl;
	cout << "int, const int: " << std::is_same<int, const int>::value << std::endl;
	cout << "int, integer_type: " << std::is_same<int, integer_type>::value << std::endl;
	cout << "A, B: " << std::is_same<A,B>::value << std::endl;
	cout << "A, C: " << std::is_same<A,C>::value << std::endl;
	cout << "signed char, std::int8_t: " << std::is_same<signed char,std::int8_t>::value << std::endl;
	*/
	printf("sizeof(double) = %lu\n", sizeof(double));
	printf("sizeof(value) = %lu\n", sizeof(Value));
	printf("sizeof(TValue) = %lu\n", sizeof(TValue));
	printf("sizeof(TKey) = %lu\n", sizeof(TKey));
	printf("sizeof(Node) = %lu\n", sizeof(Node));
	Node node = {
		{{NULL}, 0},
		{{{NULL}, 0, NULL}}
	};
	node.i_key.nk.tt = 2;
	node.i_key.nk.value.n = 100;
	printf("&node.i_key.tvk = %p\n", &node.i_key.tvk);
	printf("&node.i_key.nk = %p\n", &node.i_key.nk);
	printf("&node.i_key.tvk.value = %p\n", &node.i_key.tvk.value);
	printf("&node.i_key.nk.value = %p\n", &node.i_key.nk.value);
	printf("node.i_key.tvk.tt = %d\n", node.i_key.tvk.tt);
	printf("node.i_key.tvk.value.n = %f\n", node.i_key.tvk.value.n);
	printf("node.i_key.nk.tt = %d\n", node.i_key.nk.tt);
	printf("node.i_key.nk.value.n = %f\n", node.i_key.nk.value.n);

	tt t = {"abcdefg"};
	tt* pt = &t;
	cout << int('a') << endl;
	cout << *pt->p << endl;
	cout << int(*pt->p++) << endl;
	cout << *pt->p << endl;
    return 0;
}