#include "practice.hpp"
#include <stack>
using std::stack;
//test
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
