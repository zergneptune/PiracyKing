#include <stack>
#include <iostream>
#include "int_tree.hpp"

template<typename T>
std::shared_ptr<typename RBTree<T>::KeyNode> RBTree<T>::NIL_NODE = std::make_shared<KeyNode>(Color::BLACK, 0);

template<typename T>
RBTree<T>::RBTree()
{
}

template<typename T>
RBTree<T>::~RBTree()
{
}

template<typename T>
void RBTree<T>::insert(T& data, int low, int high)
{
    if (low > high)
    {
        std::cerr << "low > high" << std::endl;
    }
    auto p_new_node = std::make_shared<KeyNode>(data, Color::RED, low, high);
    p_new_node->max_high = high;
    p_new_node->left = NIL_NODE;
    p_new_node->right = NIL_NODE;
    decltype(p_new_node) work = m_root;
    decltype(p_new_node) temp = NIL_NODE;
    while (work != NIL_NODE)
    {
        temp = work;
        if (p_new_node->low < work->low)
        {
            work = work->left;
        }
        else
        {
            work = work->right;
        }
    }

    p_new_node->parent = temp;//设置新节点父节点

    if (temp == NIL_NODE)
    {
        m_root = p_new_node;
    }
    else if (p_new_node->low < temp->low)
    {
        temp->left = p_new_node;
    }
    else
    {
        temp->right = p_new_node;
    }
    //维护max_high
    temp = PA(p_new_node);
    while (temp != NIL_NODE)
    {
        temp->max_high = GET_MAX_HIGH(temp);
        temp = PA(temp);
    }
    
    //调整树
    insert_fixup(p_new_node);
}

template<typename T>
void RBTree<T>::insert_fixup(std::shared_ptr<KeyNode> new_node)
{
    while (PA(new_node)->color == Color::RED)//父节点是RED才需要调整
    {
        if (PA(new_node) == GRAND_PA(new_node)->left)//1. 新结点的父节点是祖父节点的[左]孩子
        {
            auto uncle = GRAND_PA(new_node)->right;
            if (uncle->color == Color::RED)//(1): 如果叔叔节点为RED
            {
                PA(new_node)->color = Color::BLACK;
                uncle->color = Color::BLACK;
                GRAND_PA(new_node)->color = Color::RED;
                new_node = GRAND_PA(new_node);
            }
            else if (new_node == PA(new_node)->right)//(2): 如果新节点是父节点的[右]孩子
            {
                new_node = PA(new_node);
                left_rotate(new_node);//左旋
            }

            if (PA(new_node) != NIL_NODE)
            {
                PA(new_node)->color = Color::BLACK;
                if (GRAND_PA(new_node) != NIL_NODE)
                {
                    GRAND_PA(new_node)->color = Color::RED;
                    right_rotate(GRAND_PA(new_node));//右旋
                }
            }
        }
        else//2. 新结点的父节点是祖父节点的右孩子
        {
            //第1种情况的镜像，交换left和right
            auto uncle = GRAND_PA(new_node)->left;
            if (uncle->color == Color::RED)//(1): 如果叔叔节点为RED
            {
                PA(new_node)->color = Color::BLACK;
                uncle->color = Color::BLACK;
                GRAND_PA(new_node)->color = Color::RED;
                new_node = GRAND_PA(new_node);
            }
            else if (new_node == PA(new_node)->left)//(2): 如果新节点是父节点的[左]孩子
            {
                new_node = PA(new_node);
                right_rotate(new_node);//右旋
            }

            if (PA(new_node) != NIL_NODE)
            {
                PA(new_node)->color = Color::BLACK;
                if (GRAND_PA(new_node) != NIL_NODE)
                {
                    GRAND_PA(new_node)->color = Color::RED;
                    left_rotate(GRAND_PA(new_node));//左旋
                }
            }
        }
    }
    m_root->color = Color::BLACK;
}

template<typename T>
void RBTree<T>::transplant(std::shared_ptr<KeyNode> u, std::shared_ptr<KeyNode> v)//v可能是NIL_NODE
{
    //std::cout << "start transplant" << std::endl;
    if (PA(u) == NIL_NODE)//1. u是根节点
    {
        m_root = v;
    }
    else if (u == PA(u)->left)//2. u是父节点的左孩子
    {
        PA(u)->left = v;
    }
    else//3. u是父节点的右孩子
    {
        PA(u)->right = v;
    }

    PA(v) = PA(u);
    //std::cout << "end transplant" << std::endl;
}

template<typename T>
bool RBTree<T>::remove(const T& data)
{
    auto del_node = search_node(m_root, data);
    if (del_node == NIL_NODE)
    {
        return false;
    }
    //std::cout << "search " << data << " start to remove"<< std::endl;
    remove_node(del_node);
    //std::cout << "end remove" << std::endl;
    return true;
}

template<typename T>
void RBTree<T>::remove_node(std::shared_ptr<KeyNode> del_node)
{
    auto temp = del_node;
    auto temp_original_color = temp->color;
    std::shared_ptr<KeyNode> fixup_node;
    if (del_node->left == NIL_NODE)//1. 待删除节点没有左孩子
    {
        fixup_node = del_node->right;
        transplant(del_node, del_node->right);
    }
    else if (del_node->right == NIL_NODE)//2. 待删除节点没有右孩子
    {
        fixup_node = del_node->left;
        transplant(del_node, del_node->left);
    }
    else//3. 待删除节点拥有左右孩子
    {
        temp = min_node(del_node->right);//找到待删除节点[右]子树中 最小的节点
        temp_original_color = temp->color;
        fixup_node = temp->right;

        if (PA(temp) == del_node)//待删除节点[右]子树根节点没有[左]孩子
        {
            PA(fixup_node) = temp;
        }
        else
        {
            transplant(temp, temp->right);
            temp->right = del_node->right;
            PA(temp->right) = temp;
        }
        transplant(del_node, temp);
        temp->left = del_node->left;
        PA(temp->left) = temp;
        temp->color = del_node->color;
    }
    //维护max_high
    temp = PA(fixup_node);
    while (temp != NIL_NODE)
    {
        temp->max_high = GET_MAX_HIGH(temp);
        temp = PA(temp);
    }

    if (temp_original_color == Color::BLACK)
    {
        //std::cout << "fixup node " << fixup_node->data << std::endl;
        remove_fixup(fixup_node);
    }
}

template<typename T>
void RBTree<T>::remove_fixup(std::shared_ptr<KeyNode> node)//node可能是NIL_NODE
{
    //std::cout << "### start fixup" << std::endl;
    while(node != m_root && node->color == Color::BLACK)
    {
        if (node == PA(node)->left)//1. node为父节点[左]孩子
        {
            auto brother = PA(node)->right;
            if (brother->color == Color::RED)//兄弟节点为RED
            {
                brother->color = Color::BLACK;
                PA(node)->color = Color::RED;
                left_rotate(PA(node));//[左]旋
                brother = PA(node)->right;
            }

            if (brother->left->color == Color::BLACK && brother->right->color == Color::BLACK)//兄弟节点的孩子都为BLACK
            {
                brother->color = Color::RED;
                node = PA(node);
            }
            else if (brother->right->color == Color::BLACK)//兄弟节点的[右]孩子为BLACK, [左]孩子为RED
            {
                brother->left->color = Color::BLACK;
                brother->color = Color::RED;
                right_rotate(brother);//[右]旋
                brother = PA(node)->right;
            }
            else
            {
                brother->color = PA(node)->color;
                PA(node)->color = Color::BLACK;
                brother->right->color = Color::BLACK;
                left_rotate(PA(node));//[左]旋
                node = m_root;//使循环终止
            }
        }
        else//2. node为父节点的[右]孩子
        {
            //是第1种情况的镜像，交换left和right
            auto brother = PA(node)->left;
            if (brother->color == Color::RED)//兄弟节点为RED
            {
                brother->color = Color::BLACK;
                PA(node)->color = Color::RED;
                right_rotate(PA(node));//[右]旋
                brother = PA(node)->left;
            }

            if (brother->left->color == Color::BLACK && brother->right->color == Color::BLACK)//兄弟节点的孩子都为BLACK
            {
                brother->color = Color::RED;
                node = PA(node);
            }
            else if (brother->left->color == Color::BLACK)//兄弟节点的[左]孩子为BLACK, [右]孩子为RED
            {
                brother->right->color = Color::BLACK;
                brother->color = Color::RED;
                left_rotate(brother);//[左]旋
                brother = PA(node)->left;
            }
            else
            {
                brother->color = PA(node)->color;
                PA(node)->color = Color::BLACK;
                brother->left->color = Color::BLACK;
                right_rotate(PA(node));//[右]旋
                node = m_root;//使循环终止
            }
        }
    }
    node->color = Color::BLACK;
    //std::cout << "### end fixup" << std::endl;
}

template<typename T>
std::shared_ptr<typename RBTree<T>::KeyNode> RBTree<T>::search_node(std::shared_ptr<KeyNode> curr_node, const T& data)
{
    while (curr_node != NIL_NODE && data != curr_node->data)
    {
        if (data < curr_node->data)
        {
            curr_node = curr_node->left;
        }
        else
        {
            curr_node = curr_node->right;
        }
    }
    return curr_node;
}

template<typename T>
void RBTree<T>::left_rotate(std::shared_ptr<KeyNode> curr_node)
{
    auto temp = curr_node->right;
    curr_node->right = temp->left;
    if (temp->left != NIL_NODE)
    {
        temp->left->parent = curr_node;
    }

    temp->parent = curr_node->parent;

    if (curr_node->parent == NIL_NODE)
    {
        m_root = temp;
    }
    else if (curr_node == curr_node->parent->left)//curr_node 是父节点的左孩子
    {
        curr_node->parent->left = temp;
    }
    else//curr_node 是父节点的右孩子
    {
        curr_node->parent->right = temp;
    }

    temp->left = curr_node;
    curr_node->parent = temp;
    //维护max_high
    temp->max_high = curr_node->max_high;
    curr_node->max_high = GET_MAX_HIGH(curr_node);
}

template<typename T>
void RBTree<T>::right_rotate(std::shared_ptr<KeyNode> curr_node)
{
    auto temp = curr_node->left;
    curr_node->left = temp->right;
    if (temp->right != NIL_NODE)
    {
        temp->right->parent = curr_node;
    }

    temp->parent = curr_node->parent;

    if (curr_node->parent == NIL_NODE)
    {
        m_root = temp;
    }
    else if (curr_node == curr_node->parent->left)//curr_node 是父节点的左孩子
    {
        curr_node->parent->left = temp;
    }
    else//curr_node 是父节点的右孩子
    {
        curr_node->parent->right = temp;
    }

    temp->right = curr_node;
    curr_node->parent = temp;
    //维护max_high
    temp->max_high = curr_node->max_high;
    curr_node->max_high = GET_MAX_HIGH(curr_node);
}

template<typename T>
void RBTree<T>::in_order()
{
    std::stack<std::shared_ptr<KeyNode>> temp_st;
    if (m_root == NIL_NODE)
    {
        return;
    }

    temp_st.push(m_root);
    while(!temp_st.empty())
    {
        auto temp_node = temp_st.top();
        if (temp_node->left != NIL_NODE)
        {
            temp_st.push(temp_node->left);
        }
        else
        {
            while (!temp_st.empty())
            {
                temp_node = temp_st.top();
                temp_st.pop();
                std::cout << temp_node->data << "(" << get_color(temp_node->color);
                std::cout << "," << temp_node->low << "," << temp_node->high << "," << temp_node->max_high << "), ";
                if (temp_node->right != NIL_NODE)
                {
                    temp_st.push(temp_node->right);
                    break;
                }
            }
        }
    }
    std::cout << std::endl;
}

template<typename T>
void RBTree<T>::in_order_r()
{
    in_order_core(m_root);
}

template<typename T>
void RBTree<T>::in_order_core(std::shared_ptr<typename RBTree<T>::KeyNode> curr_node)
{
    if (curr_node == NIL_NODE)
    {
        return;
    }

    in_order_core(curr_node->left);

    std::cout << curr_node->data << ", ";

    in_order_core(curr_node->right);
}

template<typename T>
T RBTree<T>::interval_search(int low , int high)
{
    auto p_node = int_search(m_root, low , high);
    if (p_node != NIL_NODE) {
        return p_node->data;
    } else {
        return T();
    }
}

//区间树查找
template<typename T>
std::shared_ptr<typename RBTree<T>::KeyNode> RBTree<T>::int_search(std::shared_ptr<KeyNode> curr_node, int low, int high)
{
    while (curr_node != NIL_NODE && !is_overlap(low, high, curr_node->low, curr_node->high))
    {
        if (curr_node->left != NIL_NODE && curr_node->left->max_high >= low)
        {
            curr_node = curr_node->left;
        }
        else
        {
            curr_node = curr_node->right;
        }
    }
    return curr_node;
}
