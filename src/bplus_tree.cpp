#include <filesystem>

#include "bplus_tree.hpp"

BPlusTree::BPlusTree(const std::string &file_path)
{
    this->d = MAX_INDEX_NUM;
    this->hd = d >> 1;
    this->hf = (d - 1) >> 1;
    this->d2 = MAX_DATA_NUM;
    this->hd2 = d2 >> 1;
    this->hf2 = (d2 - 1) >> 1;
    this->root = -1;
    this->cnt = 0;
    if (!std::filesystem::exists(file_path))
    {
        std::ofstream _file(file_path);
        _file.close();
    }
    this->file = std::fstream(file_path, std::ios::binary | std::ios::in | std::ios::out);
    if (!this->file.is_open())
    {
        throw std::runtime_error("Error opening file.");
    }
}

value_t BPlusTree::get(key_t key)
{
    if (root == -1)
    {
        return -1;
    }
    auto node = _search(key);
    for (int i = 0; i < node->keys.size(); ++i)
    {
        if (node->keys[i] == key)
        {
            return node->values[i];
        }
    }
    return -1;
}

void BPlusTree::insert(key_t k, value_t v)
{
    if (root == -1)
    {
        std::unique_ptr<Node> root_node(new Node(-1, true, cnt++));
        root = root_node->pos;
        writeNode(root_node);
    }
    auto node = _search(k);
    size_t i = 0;
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
        writeNode(node);
        return;
    }
    // 叶节点满了
    std::unique_ptr<Node> new_node(new Node(node->parent, true, cnt++));
    new_node->keys = std::vector<key_t>(node->keys.begin() + hd2, node->keys.end());
    new_node->values = std::vector<value_t>(node->values.begin() + hd2, node->values.end());
    node->keys.erase(node->keys.begin() + hd2, node->keys.begin() + d2);
    node->values.erase(node->values.begin() + hd2, node->values.begin() + d2);
    _add(node->parent, new_node->keys[0], node, new_node);
}
void BPlusTree::remove(key_t k)
{
    if (root == -1)
    {
        return;
    }
    auto node = _search(k);
    size_t i = 0;
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
    if (node->parent == -1)
    {
        if (node->keys.empty())
        {
            root = -1;
        }
        else
        {
            writeNode(node);
        }
        return;
    }
    _fix(node);
}

std::unique_ptr<Node> BPlusTree::_search(key_t k)
{
    // 返回可能包含 k 的叶节点
    auto node = readNode(root);
    while (!node->is_leaf)
    {
        size_t i = 0;
        bool found = false;
        for (; i < node->keys.size(); ++i)
        {
            if (k < node->keys[i])
            {
                node = readNode(node->children[i]);
                found = true;
                break;
            }
        }
        if (!found)
        {
            node = readNode(node->children.back());
        }
    }
    return node;
}

void BPlusTree::_add(page_num_t parent, key_t k, std::unique_ptr<Node> &node, std::unique_ptr<Node> &new_node)
{
    // 将键 k 上移添加到 parent 的 keys 中
    std::unique_ptr<Node> parent_node;
    if (parent == -1)
    {
        // 根节点满了，分裂
        parent_node = std::unique_ptr<Node>(new Node(-1, false, cnt++));
        root = parent_node->pos;
        parent_node->children.push_back(node->pos);
        node->parent = parent_node->pos;
    }
    else
    {
        parent_node = readNode(parent);
    }
    size_t i = 0;
    for (; i < parent_node->keys.size() && k > parent_node->keys[i]; ++i)
        ;
    parent_node->keys.insert(parent_node->keys.begin() + i, k);
    parent_node->children.insert(parent_node->children.begin() + i + 1, new_node->pos);
    new_node->parent = parent_node->pos;
    writeNode(node);
    writeNode(new_node);
    if (parent_node->keys.size() < d)
    {
        writeNode(parent_node);
        return;
    }
    _split(parent_node);
}

void BPlusTree::_split(std::unique_ptr<Node> &node)
{
    // 分裂非叶节点
    std::unique_ptr<Node> new_node(new Node(node->parent, node->is_leaf, cnt++));
    new_node->keys = std::vector<key_t>(node->keys.begin() + hd + 1, node->keys.end());
    new_node->children = std::vector<page_num_t>(node->children.begin() + hd + 1, node->children.end());
    key_t k = node->keys[hd];
    node->keys.erase(node->keys.begin() + hd, node->keys.end());
    node->children.erase(node->children.begin() + hd + 1, node->children.end());
    for (page_num_t child : new_node->children)
    {
        auto child_node = readNode(child);
        child_node->parent = new_node->pos;
        writeNode(child_node);
    }
    _add(node->parent, k, node, new_node);
}
void BPlusTree::_fix(std::unique_ptr<Node> &node)
{
    // 修复删除后的叶节点
    auto parent_node = readNode(node->parent);
    int j = 0;
    for (; j < parent_node->children.size() && parent_node->children[j] != node->pos; ++j)
        ;
    // 如果key是node第一个键，需要更新parent_node的键
    if (j > 0 && !node->keys.empty())
    {
        parent_node->keys[j - 1] = node->keys[0];
    }
    if (node->keys.size() >= hf2)
    {
        writeNode(node);
        writeNode(parent_node);
        return;
    }
    // 尝试从右兄弟借一个键
    std::unique_ptr<Node> r_sibling = nullptr;
    if (j < parent_node->keys.size())
    {
        r_sibling = readNode(parent_node->children[j + 1]);
        if (r_sibling->keys.size() > hf2)
        {
            node->keys.push_back(r_sibling->keys[0]);
            node->values.push_back(r_sibling->values[0]);
            r_sibling->keys.erase(r_sibling->keys.begin());
            r_sibling->values.erase(r_sibling->values.begin());
            parent_node->keys[j] = r_sibling->keys[0];
            writeNode(node);
            writeNode(r_sibling);
            writeNode(parent_node);
            return;
        }
    }
    // 尝试从左兄弟借一个键
    std::unique_ptr<Node> l_sibling = nullptr;
    if (j > 0)
    {
        l_sibling = readNode(parent_node->children[j - 1]);
        if (l_sibling->keys.size() > hf2)
        {
            node->keys.insert(node->keys.begin(), l_sibling->keys.back());
            node->values.insert(node->values.begin(), l_sibling->values.back());
            l_sibling->keys.pop_back();
            l_sibling->values.pop_back();
            parent_node->keys[j - 1] = node->keys[0];
            writeNode(node);
            writeNode(l_sibling);
            writeNode(parent_node);
            return;
        }
    }
    // 合并
    std::unique_ptr<Node> left, right;
    size_t left_j;
    if (r_sibling != nullptr)
    {
        left = std::move(node);
        right = std::move(r_sibling);
        left_j = j;
    }
    else
    {
        left = std::move(l_sibling);
        right = std::move(node);
        left_j = j - 1;
    }
    left->keys.insert(left->keys.end(), right->keys.begin(), right->keys.end());
    left->values.insert(left->values.end(), right->values.begin(), right->values.end());
    parent_node->keys.erase(parent_node->keys.begin() + left_j);
    parent_node->children.erase(parent_node->children.begin() + left_j + 1);
    if (parent_node->parent == -1)
    {
        if (parent_node->keys.empty())
        {
            root = left->pos;
            left->parent = -1;
        }
        else
        {
            writeNode(parent_node);
        }
        writeNode(left);
        return;
    }
    writeNode(left);
    if (parent_node->keys.size() < hf)
    {
        _merge(parent_node);
    }
}
void BPlusTree::_merge(std::unique_ptr<Node> &node)
{
    // 合并非叶节点
    auto parent_node = readNode(node->parent);
    int j = 0;
    for (; j < parent_node->children.size() && parent_node->children[j] != node->pos; ++j)
        ;
    // 尝试从右兄弟借一个键
    std::unique_ptr<Node> r_sibling = nullptr;
    if (j < parent_node->keys.size())
    {
        r_sibling = readNode(parent_node->children[j + 1]);
        if (r_sibling->keys.size() > hf)
        {
            node->keys.push_back(r_sibling->keys[0]);
            node->children.push_back(r_sibling->children[0]);
            r_sibling->keys.erase(r_sibling->keys.begin());
            r_sibling->children.erase(r_sibling->children.begin());
            parent_node->keys[j] = r_sibling->keys[0];
            writeNode(node);
            writeNode(r_sibling);
            writeNode(parent_node);
            return;
        }
    }
    // 尝试从左兄弟借一个键
    std::unique_ptr<Node> l_sibling = nullptr;
    if (j > 0)
    {
        l_sibling = readNode(parent_node->children[j - 1]);
        if (l_sibling->keys.size() > hf)
        {
            node->keys.insert(node->keys.begin(), l_sibling->keys.back());
            node->children.insert(node->children.begin(), l_sibling->children.back());
            l_sibling->keys.pop_back();
            l_sibling->values.pop_back();
            parent_node->keys[j - 1] = node->keys[0];
            writeNode(node);
            writeNode(l_sibling);
            writeNode(parent_node);
            return;
        }
    }
    std::unique_ptr<Node> left, right;
    size_t left_j;
    if (r_sibling != nullptr)
    {
        left = std::move(node);
        right = std::move(r_sibling);
        left_j = j;
    }
    else
    {
        left = std::move(l_sibling);
        right = std::move(node);
        left_j = j - 1;
    }
    left->keys.push_back(parent_node->keys[left_j]);
    left->keys.insert(left->keys.end(), right->keys.begin(), right->keys.end());
    left->children.insert(left->children.end(), right->children.begin(), right->children.end());
    for (page_num_t child : right->children)
    {
        auto child_node = readNode(child);
        child_node->parent = left->pos;
        writeNode(child_node);
    }
    parent_node->keys.erase(parent_node->keys.begin() + left_j);
    parent_node->children.erase(parent_node->children.begin() + left_j + 1);
    if (parent_node->parent == -1)
    {
        if (parent_node->keys.empty())
        {
            root = left->pos;
            left->parent = -1;
        }
        else
        {
            writeNode(parent_node);
        }
        writeNode(left);
        return;
    }
    writeNode(left);
    if (parent_node->keys.size() < hf)
    {
        _merge(parent_node);
    }
    writeNode(parent_node);
}