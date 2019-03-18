#pragma once
#include <iostream>
#include <string>
#include <list>
#include <termios.h>
using std::string;
using std::cout;
using std::endl;
using std::list;
using std::pair;

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

struct BinaryTreeNode
{
	BinaryTreeNode(int value): m_nValue(value), m_pLeft(NULL), m_pRight(NULL){}
	int m_nValue;
	BinaryTreeNode* m_pLeft;
	BinaryTreeNode* m_pRight;
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

class GetSum
{
public:
	GetSum(){ ++n; sum += n; };
	int getSum(){ return sum; }
private:
	static int sum;
	static int n;
};

void CreateDeque(BinaryTreeNode* pCurrNode, BinaryTreeNode** pDQueHead);

void PrintPath(BinaryTreeNode* pHead, int sum);

int jump(int row, int col);

bool isPostOrderOfBST(int a[], int len);

int bm_search(char* text, int text_len, char* pattern, int pattern_len);

int getSum(int n);

int getSum_2(int n);

int getSum_3(int n);

int lastNumberOfCircle(int n, int m);

int CountOfBinary1(unsigned long a);

void print(char* fmt, ...);

/*
** extern int getpagesize();
*/

char * get_program_path(char *buf,int count);

char * get_program_name(char *buf,int count);

void test_keyboard_input();

void printf_format();
