#include <cstring>
#include <algorithm>

#include "bplus_tree.hpp"

BPlusTree::BPlusTree(const std::string &file_path, bool reset)
{
    if (reset)
    {
        std::ofstream _file(file_path);
        _file.close();
        this->max_index = MAX_INDEX_NUM;
        this->max_data = MAX_DATA_NUM;
        this->root = 0;
        this->cnt = 1;
        this->file = std::fstream(file_path, std::ios::binary | std::ios::in | std::ios::out);
        if (!this->file.is_open())
        {
            throw std::runtime_error("Error opening file.");
        }
        // _write();
    }
    else
    {
        this->file = std::fstream(file_path, std::ios::binary | std::ios::in | std::ios::out);
        if (!this->file.is_open())
        {
            throw std::runtime_error("Error opening file.");
        }
        _read();
    }
    this->file_path = file_path;
    this->mid_indx = max_index >> 1;
    this->mid_data = max_data >> 1;
    this->half_index = (max_index - 1) >> 1;
    this->half_data = (max_data - 1) >> 1;
    this->to_write = std::unordered_set<page_num_t>();
    this->cost_time = 0;
}

value_t BPlusTree::get(key_t key)
{
    if (root == 0)
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
    if (root == 0)
    {
        std::unique_ptr<Node> root_node(new Node(0, true, cnt++));
        root = root_node->pos;
        // _write();
        write_node(root_node);
    }
    auto node = _search(k);
    size_t i = std::lower_bound(node->keys.begin(), node->keys.end(), k) - node->keys.begin();
    if (i < node->keys.size() && node->keys[i] == k)
    {
        node->values[i] = v;
        write_node(node);
        return;
    }
    // size_t i = 0;
    // for (; i < node->keys.size(); ++i)
    // {
    //     if (k < node->keys[i])
    //     {
    //         break;
    //     }
    //     if (k == node->keys[i])
    //     {
    //         node->values[i] = v;
    //         return;
    //     }
    // }
    node->keys.insert(node->keys.begin() + i, k);
    node->values.insert(node->values.begin() + i, v);
    if (node->keys.size() < max_data)
    {
        write_node(node);
        return;
    }
    // 叶节点满了
    std::unique_ptr<Node> new_node(new Node(node->parent, true, cnt++));
    // _write();
    new_node->keys = std::vector<key_t>(node->keys.begin() + mid_data, node->keys.end());
    new_node->values = std::vector<value_t>(node->values.begin() + mid_data, node->values.end());
    node->keys.erase(node->keys.begin() + mid_data, node->keys.begin() + max_data);
    node->values.erase(node->values.begin() + mid_data, node->values.begin() + max_data);
    _add(node->parent, new_node->keys[0], node, new_node);
}
void BPlusTree::remove(key_t k)
{
    if (root == 0)
    {
        return;
    }
    auto node = _search(k);
    size_t i = std::lower_bound(node->keys.begin(), node->keys.end(), k) - node->keys.begin();
    if (i == node->keys.size() || node->keys[i] != k)
    {
        // 不存在
        return;
    }
    // size_t i = 0;
    // for (; i < node->keys.size() && k != node->keys[i]; ++i)
    //     ;
    // if (i == node->keys.size())
    // {
    //     // 不存在
    //     return;
    // }
    node->keys.erase(node->keys.begin() + i);
    node->values.erase(node->values.begin() + i);
    // 如果node就是根节点
    if (node->parent == 0)
    {
        if (node->keys.empty())
        {
            root = 0;
            // _write();
        }
        else
        {
            write_node(node);
        }
        return;
    }
    _fix(node);
}

std::unique_ptr<Node> BPlusTree::_search(key_t k)
{
    // 返回可能包含 k 的叶节点
    auto node = read_node(root);
    while (!node->is_leaf)
    {
        size_t i = std::upper_bound(node->keys.begin(), node->keys.end(), k) - node->keys.begin();
        // size_t i = 0;
        // for (; i < node->keys.size() && k >= node->keys[i]; ++i)
        //     ;
        node = read_node(node->children[i]);
    }
    return node;
}

void BPlusTree::_add(page_num_t parent, key_t k, std::unique_ptr<Node> &node, std::unique_ptr<Node> &new_node)
{
    // 将键 k 上移添加到 parent 的 keys 中
    std::unique_ptr<Node> parent_node;
    if (parent == 0)
    {
        // 根节点满了，分裂
        parent_node = std::unique_ptr<Node>(new Node(0, false, cnt++));
        root = parent_node->pos;
        // _write();
        parent_node->children.push_back(node->pos);
        node->parent = parent_node->pos;
    }
    else
    {
        parent_node = read_node(parent);
    }
    size_t i = std::lower_bound(parent_node->keys.begin(), parent_node->keys.end(), k) - parent_node->keys.begin();
    // size_t i = 0;
    // for (; i < parent_node->keys.size() && k > parent_node->keys[i]; ++i)
    //     ;
    parent_node->keys.insert(parent_node->keys.begin() + i, k);
    parent_node->children.insert(parent_node->children.begin() + i + 1, new_node->pos);
    new_node->parent = parent_node->pos;
    write_node(node);
    write_node(new_node);
    if (parent_node->keys.size() < max_index)
    {
        write_node(parent_node);
        return;
    }
    _split(parent_node);
}

void BPlusTree::_split(std::unique_ptr<Node> &node)
{
    // 分裂非叶节点
    std::unique_ptr<Node> new_node(new Node(node->parent, node->is_leaf, cnt++));
    // _write();
    new_node->keys = std::vector<key_t>(node->keys.begin() + mid_indx + 1, node->keys.end());
    new_node->children = std::vector<page_num_t>(node->children.begin() + mid_indx + 1, node->children.end());
    key_t k = node->keys[mid_indx];
    node->keys.erase(node->keys.begin() + mid_indx, node->keys.end());
    node->children.erase(node->children.begin() + mid_indx + 1, node->children.end());
    for (page_num_t child : new_node->children)
    {
        auto child_node = read_node(child);
        child_node->parent = new_node->pos;
        write_node(child_node);
    }
    _add(node->parent, k, node, new_node);
}

void BPlusTree::_fix(std::unique_ptr<Node> &node)
{
    // 修复删除后的叶节点
    auto parent_node = read_node(node->parent);
    size_t j = std::lower_bound(parent_node->children.begin(), parent_node->children.end(), node->pos) - parent_node->children.begin();
    // int j = 0;
    // for (; j < parent_node->children.size() && parent_node->children[j] != node->pos; ++j)
    //     ;
    // 如果key是node第一个键，需要更新parent_node的键
    if (j > 0 && !node->keys.empty())
    {
        parent_node->keys[j - 1] = node->keys[0];
    }
    if (node->keys.size() >= half_data)
    {
        write_node(node);
        write_node(parent_node);
        return;
    }
    // 尝试从右兄弟借一个键
    std::unique_ptr<Node> r_sibling = nullptr;
    if (j < parent_node->keys.size())
    {
        r_sibling = read_node(parent_node->children[j + 1]);
        if (r_sibling->keys.size() > half_data)
        {
            node->keys.push_back(r_sibling->keys[0]);
            node->values.push_back(r_sibling->values[0]);
            r_sibling->keys.erase(r_sibling->keys.begin());
            r_sibling->values.erase(r_sibling->values.begin());
            parent_node->keys[j] = r_sibling->keys[0];
            write_node(node);
            write_node(r_sibling);
            write_node(parent_node);
            return;
        }
    }
    // 尝试从左兄弟借一个键
    std::unique_ptr<Node> l_sibling = nullptr;
    if (j > 0)
    {
        l_sibling = read_node(parent_node->children[j - 1]);
        if (l_sibling->keys.size() > half_data)
        {
            node->keys.insert(node->keys.begin(), l_sibling->keys.back());
            node->values.insert(node->values.begin(), l_sibling->values.back());
            l_sibling->keys.pop_back();
            l_sibling->values.pop_back();
            parent_node->keys[j - 1] = node->keys[0];
            write_node(node);
            write_node(l_sibling);
            write_node(parent_node);
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
    if (parent_node->parent == 0)
    {
        if (parent_node->keys.empty())
        {
            root = left->pos;
            // _write();
            left->parent = 0;
        }
        else
        {
            write_node(parent_node);
        }
        write_node(left);
        return;
    }
    write_node(left);
    if (parent_node->keys.size() < half_index)
    {
        _merge(parent_node);
    }
}
void BPlusTree::_merge(std::unique_ptr<Node> &node)
{
    // 合并非叶节点
    auto parent_node = read_node(node->parent);
    size_t j = std::lower_bound(parent_node->children.begin(), parent_node->children.end(), node->pos) - parent_node->children.begin();
    // int j = 0;
    // for (; j < parent_node->children.size() && parent_node->children[j] != node->pos; ++j)
    //     ;
    // 尝试从右兄弟借一个键
    std::unique_ptr<Node> r_sibling = nullptr;
    if (j < parent_node->keys.size())
    {
        r_sibling = read_node(parent_node->children[j + 1]);
        if (r_sibling->keys.size() > half_index)
        {
            node->keys.push_back(r_sibling->keys[0]);
            node->children.push_back(r_sibling->children[0]);
            r_sibling->keys.erase(r_sibling->keys.begin());
            r_sibling->children.erase(r_sibling->children.begin());
            parent_node->keys[j] = r_sibling->keys[0];
            write_node(node);
            write_node(r_sibling);
            write_node(parent_node);
            return;
        }
    }
    // 尝试从左兄弟借一个键
    std::unique_ptr<Node> l_sibling = nullptr;
    if (j > 0)
    {
        l_sibling = read_node(parent_node->children[j - 1]);
        if (l_sibling->keys.size() > half_index)
        {
            node->keys.insert(node->keys.begin(), l_sibling->keys.back());
            node->children.insert(node->children.begin(), l_sibling->children.back());
            l_sibling->keys.pop_back();
            l_sibling->values.pop_back();
            parent_node->keys[j - 1] = node->keys[0];
            write_node(node);
            write_node(l_sibling);
            write_node(parent_node);
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
        auto child_node = read_node(child);
        child_node->parent = left->pos;
        write_node(child_node);
    }
    parent_node->keys.erase(parent_node->keys.begin() + left_j);
    parent_node->children.erase(parent_node->children.begin() + left_j + 1);
    if (parent_node->parent == 0)
    {
        if (parent_node->keys.empty())
        {
            root = left->pos;
            left->parent = 0;
            // _write();
        }
        else
        {
            write_node(parent_node);
        }
        write_node(left);
        return;
    }
    write_node(left);
    if (parent_node->keys.size() < half_index)
    {
        _merge(parent_node);
    }
    write_node(parent_node);
}