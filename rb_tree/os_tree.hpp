#pragma once
#include <memory>
#include <string>
//获取父节点
#define PA(node) node->parent

//获取祖父节点
#define GRAND_PA(node) node->parent->parent

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
        Node(T v, Color c, int s) : data(v), color(c), size(s) {}
        Node(Color c, int s) : color(c), size(s) {}
        ~Node(){}

        T               		data;
        std::shared_ptr<Node>	left;
        std::shared_ptr<Node>	right;
        std::shared_ptr<Node>	parent;
        Color                   color;
        int                     size;//表示子树的大小（包含自己）
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
	void insert(T& data);

	//删除
	bool remove(const T& data);

    //查找第K小的关键字
    std::shared_ptr<KeyNode> search_k(int k);

    //中序遍历
    void in_order();
    void in_order_r();
    void in_order_core(std::shared_ptr<KeyNode> curr_node);

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
