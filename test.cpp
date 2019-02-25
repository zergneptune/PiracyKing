#include <type_traits>
#include <fstream>
#include <sstream>
#include <cstdint>
#include <thread>
#include <chrono>
#include<stdio.h>
#include<termios.h>
#include<fcntl.h>
 #include <sys/types.h>
 #include <sys/uio.h>
 #include <unistd.h>
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

////////////////////////////////////////////////////////////////////

void keyboard()
{
    fd_set rfds, rs;
    struct timeval tv;

    int i,r,q,j,dir;
    struct termios saveterm, nt;
    char c,buf[32],str[8];

    tcgetattr(0, &saveterm);
    nt = saveterm;

    nt.c_lflag &= ~ECHO;
    nt.c_lflag &= ~ISIG;   
    nt.c_lflag &= ~ICANON; 

    tcsetattr(0, TCSANOW, &nt);

    FD_ZERO(&rs);
    FD_SET(0, &rs);
    FD_ZERO(&rfds);
    FD_SET(0, &rfds);
    tv.tv_sec=0;
    tv.tv_usec=0;

    i = 0; q = 0; dir = 0;
    while(1)
    {
        read(0 , buf+i, 1);
        sprintf(str, "<%X>", *(buf+i));
        i++;
        if(i>31)
        {
            write(1,"Too many data\n",14);
            break;
        }
        write(1, str, 4);
        r = select(0 + 1, &rfds, NULL, NULL, &tv); //0：监听标准输入，若r=1，说明标准输入可读，rfds中标准输入文件描述符会就绪
        if(r<0)
        {
            write(1,"select() error.\n",16);
            break;
        }
        if(r == 1)
            continue;
        write(1, "\t", 1);
        rfds = rs; //恢复rfds，即清除就绪的标准输入文件描述符
        if(i == 3 && buf[0] == 0x1b && buf[1] == 0x5b)
        {
            c = buf[2];
            switch(c)
            {
                case 0x41:
                    write(1, "上", 3);
                    break;
                case 0x42:
                    write(1, "下", 3);
                    break;
                case 0x43:
                    write(1, "右", 3);
                    break;
                case 0x44:
                    write(1, "左", 3);
                    break;
                default:
                    break;
            }
        }
        write(1, "\n", 1);
        //确保两次连续的按下ESC键，才退出
        if(buf[0] == 27 && i == 1)
        {
            if(q == 0)
                q = 1;
            else
                break;
        }
        else
            q = 0;
        i = 0;
    }

    tcsetattr(0, TCSANOW, &saveterm);
    printf("\n");
}

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
    keyboard();
    return 0;
}
