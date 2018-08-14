#include <type_traits>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdint>
#include <list>
using std::string;
using std::cout;
using std::cerr;
using std::endl;
using std::list;
using std::pair;


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

class MyDate
{
public:
	MyDate() = default;
	MyDate(int year, int month, int day): m_year(year), m_month(month), m_day(day){}
	string format_date(string format = "yyyymmdd")
	{
		char buf[16] = {0};
		sprintf(buf, "%04d-%02d-%02d", m_year, m_month, m_day);
		return string(buf);
	}

	MyDate& operator =(MyDate& one_date)
	{
		this->m_year = one_date.m_year;
		this->m_month = one_date.m_month;
		this->m_day = one_date.m_day;
		return *this;
	}

	MyDate& operator ++()
	{
		++ m_day;
		if(m_day > getMaxDayOfMonth())
		{
			m_day = 1;
			++ m_month;
			if(m_month > 12)
			{
				m_month = 1;
				++ m_year;
			}
		}
		return *this;
	}

	MyDate operator ++(int)
	{
		MyDate prev = *this;
		return ++ (*this); 		
	}

	MyDate& calc_date(int pass_days)
	{
		for(int i = 0; i < pass_days; ++i)
		{
			++ (*this);
		}

		return *this;
	}

protected:
	bool isLeadYear()
	{
		return ((m_year % 400 == 0 || m_year % 4 == 0) && m_year % 100 != 0);
	}

	int getMaxDayOfMonth()
	{
		switch(m_month)
		{
			case 1:
			case 3:
			case 5:
			case 7:
			case 8:
			case 10:
			case 12: return 31;
			case 4:
			case 6:
			case 9:
			case 11: return 30;
			case 2: return (isLeadYear()?29:28);
		}
		return 0;
	}

private:
	int m_year{0};
	int m_month{0};
	int m_day{0};
};

int jump(int row, int col)
{
	static int arr[4][5] = {
		0, 1, 2, 3, 4,
		1, 2, 3, 4, 5,
		2, 3, 4, 5, 6,
		3, 4, 5, 6, 7
	};
	static std::list<pair<int, int>> lst_path;
	static int res_cnt = 0;

	if(row > 3 || col > 4)
	{
		return res_cnt;
	}

	lst_path.push_back(std::make_pair(row, col));

	if(arr[row][col] == 7)
	{
		cout << "path:\n";
		for(const auto& ref : lst_path)
		{
			cout << "\t" << ref.first << ", " << ref.second << "\n";
		}
		lst_path.pop_back();
		++ res_cnt;
		return res_cnt;
	}

	jump(row + 1, col);

	jump(row, col + 1);

	lst_path.pop_back();

	return res_cnt;
}

struct BSTreeNode
{
	BSTreeNode(){};
	BSTreeNode(int value, BSTreeNode* left = NULL, BSTreeNode* right = NULL): m_nValue(value), m_pLeft(left), m_pRight(right){}
	int m_nValue;
	BSTreeNode* m_pLeft;
	BSTreeNode* m_pRight;
};

void CreateDeque(BSTreeNode* pCurrNode, BSTreeNode** pDQueHead)
{
	static BSTreeNode* pPreNode = NULL;
	if(pCurrNode == NULL)
	{
		return;
	}
	if(pCurrNode->m_pLeft != NULL)
	{
		CreateDeque(pCurrNode->m_pLeft, pDQueHead);
	}
	if(pPreNode == NULL)
	{
		pPreNode = pCurrNode;
		*pDQueHead = pCurrNode;
		cout << "-" << pCurrNode->m_nValue << endl;
	}
	else
	{
		pCurrNode->m_pLeft = pPreNode;
		pPreNode->m_pRight = pCurrNode;
		pPreNode = pCurrNode;
		cout << "*" << pCurrNode->m_nValue << endl;
	}
	if(pCurrNode->m_pRight != NULL)
	{
		CreateDeque(pCurrNode->m_pRight, pDQueHead);
	}
}

template<typename T>
class MyStack
{
public:
	MyStack(): m_pData(NULL), m_nMinIndex(0), m_nTopIndex(0), m_nDataLen(0), m_nCapacity(0){}

	T min()
	{
		if(m_pData != NULL)
		{
			return T();
		}

		return m_pData[m_nMinIndex];
	}

	void push(T& t)
	{
		if(m_pData == NULL)
		{
			m_pData = static_cast<T*>(calloc(2, sizeof(T)));
			m_nCapacity = 2;
			m_nDataLen = 0;
			m_nTopIndex = 0;
			m_nMinIndex = 0;
		}

		if(m_nDataLen >= m_nCapacity)
		{
			cout << "realloc memory" << endl;
			void* pRes = realloc(m_pData, 2 * m_nCapacity * sizeof(T));
			if(pRes == NULL)
			{
				return;
			}

			m_nCapacity = 2 * m_nCapacity;
			m_pData = static_cast<T*>(pRes);
		}

		m_pData[m_nTopIndex] = t;
		++ m_nDataLen;

		if( m_pData[m_nTopIndex] < m_pData[m_nMinIndex])
		{
			m_nMinIndex = m_nTopIndex;
		}

		++m_nTopIndex;
	}

	void pop()
	{
		if( m_pData == NULL)
		{
			return;
		}

		-- m_nTopIndex;
		-- m_nDataLen;
	}

	T top()
	{
		if(m_pData == NULL)
		{
			return T();
		}

		return m_pData[m_nTopIndex - 1];
	}

	bool empty()
	{
		return m_nDataLen == 0;
	}

private:
	T* m_pData;
	int m_nTopIndex;
	int m_nMinIndex;
	int m_nDataLen;
	int m_nCapacity;
};

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
	MyStack<int> st_int;
	for(int i = 0; i < 128; ++i)
	{
		st_int.push(i);
	}

	cout << st_int.min() << endl;

	while(!st_int.empty())
	{
		cout << st_int.top() << ',';
		st_int.pop();
	}
	cout << endl;

	return 0;
}

