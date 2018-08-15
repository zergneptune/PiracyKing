#include "practice.hpp"
#include <stack>
using std::stack;

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