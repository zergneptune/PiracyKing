#include "practice.hpp"
#include <stack>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
using std::stack;
// test5
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

void CreateDeque(BinaryTreeNode* pCurrNode, BinaryTreeNode** pDQueHead)
{
	static BinaryTreeNode* pPreNode = NULL;
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

void core(BinaryTreeNode* pNode, int sum)
{
	static int cnt = 0;
	static stack<BinaryTreeNode*> st_node;

	if(pNode == NULL)
	{
		return;
	}
	cnt += pNode->m_nValue;
	st_node.push(pNode);
	cout << "value: " << pNode->m_nValue << ", sum: " << cnt << endl;

	if(pNode->m_pLeft == NULL && pNode->m_pRight == NULL)
	{
		if(cnt == sum)
		{
			cout << "path: ";
			stack<BinaryTreeNode*> st_tmp(st_node);
			while(!st_tmp.empty())
			{
				BinaryTreeNode* pTmp = st_tmp.top();
				cout << pTmp->m_nValue << ',';
				st_tmp.pop();
			}
			cout << endl;
		}
		cnt -= pNode->m_nValue;
		st_node.pop();
		return;
	}

	core(pNode->m_pLeft, sum);

	core(pNode->m_pRight, sum);

	cnt -= pNode->m_nValue;
	st_node.pop();
}

void PrintPath(BinaryTreeNode* pHead, int sum)
{
	core(pHead, sum);
}

bool helper_isPostOrderOfBST(int a[], int low, int high)
{
	int rootNode = a[high];
	int i = low;
	bool lc_tree, rc_tree;

	if(low >= high)
	{
		return true;
	}

	for(; i <= high && a[i] < rootNode; ++i){}
	if(i <= high)
	{
		lc_tree = helper_isPostOrderOfBST(a, low, i - 1);
	}

	int save_i = i;
	for(; i <= high; ++i)
	{
		if(a[i] < rootNode)
		{
			return false;
		}
	}
	rc_tree = helper_isPostOrderOfBST(a, save_i, high - 1);

	return lc_tree && rc_tree;
}

bool isPostOrderOfBST(int a[], int len)
{
	return helper_isPostOrderOfBST(a, 0, len - 1);
}

int* CreateBC(char* pattern, int len)
{
	int* bc = new int[256];

	for(int i = 0; i < 256; ++i)
	{
		bc[i] = -1;
	}

	for(int i = 0; i < len; ++i)
	{
		bc[pattern[i]] = i;
	}

	return bc;
}

int* CreateSuffix(char* pattern, int len)
{
	int* suffix = new int[len];
	suffix[len - 1] = len;

	for(int i = len - 2; i >= 0; --i)
	{
		int j = i;
		for(; pattern[j] == pattern[len - 1 - i + j] && j >= 0; --j);
		suffix[i] = i - j;
	}

	return suffix;
}

int* CreateGS(char* pattern, int len)
{
	int* suffix = CreateSuffix(pattern, len);
	int* gs = new int[len];
	/*
	在计算gs数组时，从移动数最大的情况依次到移动数最少的情况赋值，
	确保在合理的移动范围内，移动最少的距离，避免失配的情况。
	*/
	for(int i = 1; i < len; ++i)
	{
		gs[i] = len;
	}

	for(int i = len - 1; i >= 0; --i) //从右往左扫描，确保模式串移动最少。
	{
		if(suffix[i] == i + 1) //是一个与好后缀匹配的最大前缀
		{
			for(int j = 0; j < len - 1 - i; ++j)
			{
				if(gs[j] == len) //gs[j]初始值为len, 这样确保gs[j]只被修改一次
				{
					gs[j] = len - 1 - i;
				}
			}
		}
	}

	for(int i = 0; i < len - 1; ++i)
	{
		gs[len - 1 - suffix[i]] = len - 1 - i;
	}

	return gs;
}

int bm_search(char* text, int text_len, char* pattern, int pattern_len)
{
	int* bc = CreateBC(pattern, pattern_len);
	int* gs = CreateGS(pattern, pattern_len);

	for(int i = 0; i <= text_len - pattern_len; )
	{
		cout << "i = " << i << endl;
		int j = pattern_len - 1;
		for(; j >= 0 && pattern[j] == text[i+j]; --j);

		if(j < 0)
		{
			return i;
		}

		int bad_char_index = j;
		char bad_char = text[i + j];

		int bc_move = bad_char_index - bc[bad_char];
		if(bc_move < 0)
		{
			bc_move = bad_char_index + 1;
		}

		int gs_move = gs[bad_char_index];

		int move = (bc_move > gs_move ? bc_move : gs_move);

		i += move;
	}

	if(bc != NULL)
	{
		delete bc;
		bc = NULL;
	}

	if(gs != NULL)
	{
		delete bc;
		gs = NULL;
	}
	return -1;
}

int getSum(int n)
{
	int tmpt = n;
	bool bRet = (tmpt > 0) && (n += getSum(n-1));
	return n;
}

int (*getSumFunc[2])(int n);
int fun0(int n){ return 0; }
int fun1(int n){ return n + getSumFunc[!!n](n-1); }
int getSum_2(int n)
{
	getSumFunc[0] = fun0;
	getSumFunc[1] = fun1;

	int nRet = fun1(100);
	return nRet;
}

void T1(int& X, int Y, int i)
{
	bool bRet = (Y & (1<<i)) && (X+=(Y<<i));
}

int getSum_3(int n)
{
	#define T(X, Y, i) (Y & (1<<i)) && (X+=(Y<<i))

	int temp = n;
	T(temp, n, 0);T(temp, n, 1);T(temp, n, 2);T(temp, n, 3);
	T(temp, n, 4);T(temp, n, 5);T(temp, n, 6);T(temp, n, 7);
	T(temp, n, 8);T(temp, n, 9);T(temp, n, 10);T(temp, n, 11);
	T(temp, n, 12);T(temp, n, 13);T(temp, n, 14);T(temp, n, 15);
	T(temp, n, 16);T(temp, n, 17);T(temp, n, 18);T(temp, n, 19);
	T(temp, n, 20);T(temp, n, 21);T(temp, n, 22);T(temp, n, 23);
	T(temp, n, 24);T(temp, n, 25);T(temp, n, 26);T(temp, n, 27);
	T(temp, n, 28);T(temp, n, 29);T(temp, n, 30);T(temp, n, 31);
	return temp>>1;
}

int GetSum::sum = 0;
int GetSum::n = 0;

int lastNumberOfCircle(int n, int m)
{
	if(n == 1)
	{
		return 0;
	}
	return (lastNumberOfCircle(n-1, m) + m) % n;
}

int CountOfBinary1(unsigned long a)
{
	int res = 0;
	while(a)
	{
		a = (a - 1) & a;
		++ res;
	}
	return res;
}

void print(char* fmt, ...)
{

	va_list argptr;
	va_start(argptr, fmt);
	for(char* p = fmt; *p != '\0'; ++p)
	{
		if(*p == '%' && *(p+1) != '%')
		{
			if(*(p+1) == 'd')
			{
				int tmp = va_arg(argptr, int);
				cout << tmp;
				++p;
			}
			else if(*(p+1) == 's')
			{
				const char* tmp = va_arg(argptr, const char*);
				cout << tmp;
				++p;
			}
			else
			{
				cout << *p << *(p+1);
				++p;
			}
		}
		else
		{
			cout << *p;
		}
	}
	va_end(argptr);
}

/*
 * 获取程序的路径名称
 */
char * get_program_path(char *buf,int count)
{
	int i=0;
	int retval = readlink("/proc/self/exe",buf,count-1);
	if((retval < 0 || retval >= count - 1))
	{
	    return NULL;
	}
	//添加末尾结束符号
	buf[retval] = '\0';
	char *end = strrchr(buf,'/');
	if(NULL == end)
	    buf[0] = '\0';
	else
	    *end = '\0';
	return buf;
}

/*
 * 获取这个程序的文件名,其实这有点多余,argv[0] 
 * 就代表这个执行的程序文件名
 */
char * get_program_name(char *buf,int count)
{
	int retval = readlink("/proc/self/exe",buf,count-1);
	if((retval < 0 || retval >= count - 1))
	{
	    return NULL;
	}
	buf[retval] = '\0';
	//获取指向最后一次出现'/'字符的指针
	return strrchr(buf,'/');
}

/*
 * 测试键盘输入
 */
void test_keyboard_input()
{
	fd_set rfds, rs;
	struct timeval tv;

	int i,r,q,j;
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

	i=0; q=0;
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
		for(j=0; j < i; j++)
		{
			c = buf[j];
			switch(c)
			{
			   	case 27: 
			   		write(1,"ESC ",4);
			        break;
			   	case 9: 
			   		write(1,"TAB ",4);
			        break;
			   	case 32: 
			   		write(1,"SPACE ",6);
			        break;
			    default:
			    	if(c>=32 && c<127)
			    		write(1, buf+j, 1);
			       	else
			       		write(1,"CTRL ",5);
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

void printf_format()
{
	printf("\033[30m黑\033[m\n");
    printf("\033[31m红\033[m\n");
    printf("\033[32m绿\033[m\n");
    printf("\033[33m黄\033[m\n");
    printf("\033[34m蓝\033[m\n");
    printf("\033[35m紫\033[m\n");
    printf("\033[36m深绿\033[m\n");
    printf("\033[37m白\033[m\n");

    printf("\033[40m背景黑\033[m\n");
    printf("\033[41m背景红\033[m\n");
    printf("\033[42m背景绿\033[m\n");
    printf("\033[43m背景黄\033[m\n");
    printf("\033[44m背景蓝\033[m\n");
    printf("\033[45m背景紫\033[m\n");
    printf("\033[46m背景深绿\033[m\n");
    printf("\033[47m背景白\033[m\n");


    printf("\033[1;31m红,高亮\033[m\n");
    printf("\033[4;31m红,下划线\033[m\n");
    printf("\033[5;31m红,闪烁\033[m\n");
    printf("\033[7;31m红,反显\033[m\n");
    printf("\033[8;31m红,消隐\033[m\n");
}
