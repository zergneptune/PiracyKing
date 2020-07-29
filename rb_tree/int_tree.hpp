#pragma once
#include <memory>
#include <string>
#include <algorithm>
//获取父节点
#define PA(node) node->parent

//获取祖父节点
#define GRAND_PA(node) node->parent->parent

//计算节点的子树中最大的区间右端点值
#define GET_MAX_HIGH(node) std::max({node->left->max_high, node->right->max_high, node->high})

template<typename T>
class RBTree
{
    enum class Color
    {
        RED = 0,
        BLACK = 1
    };

    struct Node
    {
        Node(){}
        Node(T v, Color c, int l, int h) : data(v), color(c), low(l), high(h) {}
        Node(Color c, int max_h) : color(c), max_high(max_h) {}
        ~Node(){}

        T               		data;
        std::shared_ptr<Node>	left;
        std::shared_ptr<Node>	right;
        std::shared_ptr<Node>	parent;
        Color                   color;
        int                     low;//区间左端点
        int                     high;//区间右端点
        int                     max_high;//子树中最大的区间右端点值
    };

    using KeyNode = Node;
    /*
    ** 哨兵节点颜色为BLACK。left、right、parent不关心，但是在代码逻辑中可能会修改到
    ** 哨兵节点的存在时便于代码处理边界条件
    */
    static std::shared_ptr<KeyNode> NIL_NODE;
public:
	RBTree();

	~RBTree();

	//插入
	void insert(T& data, int low, int high);

	//删除
	bool remove(const T& data);

    //中序遍历
    void in_order();
    void in_order_r();
    void in_order_core(std::shared_ptr<KeyNode> curr_node);

    //区间树的查找：用来找出树中与区间[low, high]有重叠的一个点
    T interval_search(int low , int high);

private:
	//左旋
	void left_rotate(std::shared_ptr<KeyNode> curr_node);

	//右旋
	void right_rotate(std::shared_ptr<KeyNode> curr_node);

    //插入调整
    void insert_fixup(std::shared_ptr<KeyNode> new_node);

    //删除节点的辅助函数
    void transplant(std::shared_ptr<KeyNode> u, std::shared_ptr<KeyNode> v);

    //删除调整
    void remove_fixup(std::shared_ptr<KeyNode> node);

    //删除节点
    void remove_node(std::shared_ptr<KeyNode> del_node);
    
    //以当前节点为根查找
    std::shared_ptr<KeyNode> search_node(std::shared_ptr<KeyNode> curr_node, const T& data);

    //区间树查找
    std::shared_ptr<KeyNode> int_search(std::shared_ptr<KeyNode> curr_node, int low, int high);
    
    //比较2个区间是否有重叠
    bool is_overlap(int low_1, int high_1, int low_2, int high_2)
    {
        if (high_1 < low_2) {
            return false;
        } else if (high_2 < low_1) {
            return false;
        } else {
            return true;
        }
    }

private:
     //获取最大值节点
     std::shared_ptr<KeyNode> max_node(std::shared_ptr<KeyNode> curr_node) const
     {
         while (curr_node->right != NIL_NODE) curr_node = curr_node->right;
         return curr_node;
     }

     //获取最小值节点
     std::shared_ptr<KeyNode> min_node(std::shared_ptr<KeyNode> curr_node) const
     {
         while (curr_node->left != NIL_NODE) curr_node = curr_node->left;
         return curr_node;
     }

     std::string get_color(Color c)
     {
         if (c == Color::RED)
         {
             return "red";
         }
         else
         {
             return "black";
         }
     }

private:
	std::shared_ptr<KeyNode>	m_root = NIL_NODE;
};
