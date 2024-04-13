#include "bplus_tree.hpp"

BPlusTree::BPlusTree(int d, int d2) : d(d), hd(d >> 1), hf((d - 1) >> 1), d2(d2), hd2(d2 >> 1), hf2((d2 - 1) >> 1), root(nullptr) {}

int BPlusTree::get(int key)
{
    if (root == nullptr)
    {
        return -1;
    }
    Node *node = _search(key);
    for (int i = 0; i < node->keys.size(); ++i)
    {
        if (node->keys[i] == key)
        {
            return node->values[i];
        }
    }
    return -1;
}

void BPlusTree::insert(int k, int v)
{
    if (root == nullptr)
    {
        root = new Node(nullptr, true);
        root->is_leaf = true;
    }
    Node *node = _search(k);
    int i = 0;
    for (; i < node->keys.size(); ++i)
    {
        if (k < node->keys[i])
        {
            break;
        }
        if (k == node->keys[i])
        {
            node->values[i] = v;
            return;
        }
    }
    node->keys.insert(node->keys.begin() + i, k);
    node->values.insert(node->values.begin() + i, v);
    if (node->keys.size() < d2)
    {
        return;
    }
    // 叶节点满了
    Node *new_node = new Node(nullptr, true);
    new_node->keys = std::vector<int>(node->keys.begin() + hd2, node->keys.end());
    new_node->values = std::vector<int>(node->values.begin() + hd2, node->values.end());
    node->keys.erase(node->keys.begin() + hd2, node->keys.end());
    node->values.erase(node->values.begin() + hd2, node->values.end());
    _add(node->parent, new_node->keys[0], node, new_node);
}
void BPlusTree::remove(int k)
{
    if (root == nullptr)
    {
        return;
    }
    Node *node = _search(k);
    int i = 0;
    for (; i < node->keys.size() && k != node->keys[i]; ++i)
        ;
    if (i == node->keys.size())
    {
        // 不存在
        return;
    }
    node->keys.erase(node->keys.begin() + i);
    node->values.erase(node->values.begin() + i);
    // 如果node就是根节点
    if (node->parent == nullptr)
    {
        if (node->keys.empty())
        {
            delete node;
            root = nullptr;
        }
        return;
    }
    _fix(node);
}

Node *BPlusTree::_search(int k)
{
    // 返回可能包含 k 的叶节点
    Node *node = root;
    while (!node->is_leaf)
    {
        int i = 0;
        bool found = false;
        for (; i < node->keys.size(); ++i)
        {
            if (k < node->keys[i])
            {
                node = node->children[i];
                found = true;
                break;
            }
        }
        if (!found)
        {
            node = node->children.back();
        }
    }
    return node;
}

void BPlusTree::_split(Node *node)
{
    // 分裂非叶节点
    Node *new_node = new Node(node->parent, node->is_leaf);
    new_node->keys = std::vector<int>(node->keys.begin() + hd + 1, node->keys.end());
    new_node->children = std::vector<Node *>(node->children.begin() + hd + 1, node->children.end());
    int k = node->keys[hd];
    node->keys.erase(node->keys.begin() + hd, node->keys.end());
    node->children.erase(node->children.begin() + hd + 1, node->children.end());
    for (auto child : new_node->children)
    {
        child->parent = new_node;
    }
    _add(node->parent, k, node, new_node);
}
void BPlusTree::_add(Node *parent, int k, Node *node, Node *new_node)
{
    // 将键 k 上移添加到 parent 的 keys 中
    if (parent == nullptr)
    {
        // 根节点满了，分裂
        parent = new Node(nullptr, false);
        root = parent;
        parent->children.push_back(node);
        node->parent = parent;
    }
    int i = 0;
    for (; i < parent->keys.size() && k > parent->keys[i]; ++i)
        ;
    parent->keys.insert(parent->keys.begin() + i, k);
    parent->children.insert(parent->children.begin() + i + 1, new_node);
    new_node->parent = parent;
    if (parent->keys.size() < d)
    {
        return;
    }
    _split(parent);
}
void BPlusTree::_fix(Node *node)
{
    // 修复删除后的节点
    Node *parent = node->parent;
    int j = 0;
    for (; j < parent->children.size() && parent->children[j] != node; ++j)
        ;
    // 如果node是第一个子节点，需要更新父节点的键
    if (j > 0 && !node->keys.empty())
    {
        parent->keys[j - 1] = node->keys[0];
    }
    if (node->keys.size() >= hf2)
    {
        return;
    }
    // 尝试从右兄弟借一个键
    if (j < parent->keys.size() && parent->children[j + 1]->keys.size() > hf2)
    {
        Node *sibling = parent->children[j + 1];
        node->keys.push_back(sibling->keys[0]);
        node->values.push_back(sibling->values[0]);
        sibling->keys.erase(sibling->keys.begin());
        sibling->values.erase(sibling->values.begin());
        parent->keys[j] = sibling->keys[0];
        return;
    }
    // 尝试从左兄弟借一个键
    if (j > 0 && parent->children[j - 1]->keys.size() > hf2)
    {
        Node *sibling = parent->children[j - 1];
        node->keys.insert(node->keys.begin(), sibling->keys.back());
        node->values.insert(node->values.begin(), sibling->values.back());
        sibling->keys.pop_back();
        sibling->values.pop_back();
        parent->keys[j - 1] = node->keys[0];
        return;
    }
    // 合并
    Node *left, *right;
    int left_j;
    if (j < parent->keys.size())
    {
        left = node;
        right = parent->children[j + 1];
        left_j = j;
    }
    else
    {
        left = parent->children[j - 1];
        right = node;
        left_j = j - 1;
    }
    left->keys.insert(left->keys.end(), right->keys.begin(), right->keys.end());
    left->values.insert(left->values.end(), right->values.begin(), right->values.end());
    parent->keys.erase(parent->keys.begin() + left_j);
    parent->children.erase(parent->children.begin() + left_j + 1);
    delete right;
    if (parent == root)
    {
        if (parent->keys.empty())
        {
            root = left;
            left->parent = nullptr;
            delete parent;
        }
        return;
    }
    if (parent->keys.size() < hf)
    {
        _merge(parent);
    }
}
void BPlusTree::_merge(Node *node)
{
    // 合并非叶节点
    Node *parent = node->parent;
    int j = 0;
    for (; j < parent->children.size() && parent->children[j] != node; ++j)
        ;
    if (j < parent->keys.size() && parent->children[j + 1]->keys.size() > hf)
    {
        Node *sibling = parent->children[j + 1];
        node->keys.push_back(sibling->keys[0]);
        node->children.push_back(sibling->children[0]);
        sibling->keys.erase(sibling->keys.begin());
        sibling->children.erase(sibling->children.begin());
        parent->keys[j] = sibling->keys[0];
        return;
    }
    if (j > 0 && parent->children[j - 1]->keys.size() > hf)
    {
        Node *sibling = parent->children[j - 1];
        node->keys.insert(node->keys.begin(), sibling->keys.back());
        node->children.insert(node->children.begin(), sibling->children.back());
        sibling->keys.pop_back();
        sibling->values.pop_back();
        parent->keys[j - 1] = node->keys[0];
        return;
    }
    Node *left, *right;
    int left_j;
    if (j < parent->keys.size())
    {
        left = node;
        right = parent->children[j + 1];
        left_j = j;
    }
    else
    {
        left = parent->children[j - 1];
        right = node;
        left_j = j - 1;
    }
    left->keys.push_back(parent->keys[left_j]);
    left->keys.insert(left->keys.end(), right->keys.begin(), right->keys.end());
    left->children.insert(left->children.end(), right->children.begin(), right->children.end());
    for (auto child : right->children)
    {
        child->parent = left;
    }
    parent->keys.erase(parent->keys.begin() + left_j);
    parent->children.erase(parent->children.begin() + left_j + 1);
    delete right;
    if (parent == root)
    {
        if (parent->keys.empty())
        {
            root = left;
            left->parent = nullptr;
            delete parent;
        }
        return;
    }
    if (parent->keys.size() < hf)
    {
        _merge(parent);
    }
}