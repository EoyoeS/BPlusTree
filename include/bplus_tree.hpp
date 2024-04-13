#ifndef BPLUSTREE_HPP
#define BPLUSTREE_HPP

#include <iostream>
#include <vector>

// 节点结构体
struct Node
{
    std::vector<int> keys;        // 键值列表
    std::vector<int> values;      // 值列表
    std::vector<Node *> children; // 子节点
    bool is_leaf;                 // 是否为叶节点
    Node *parent;                 // 父节点指针

    Node(Node *parent, bool is_leaf) : parent(parent), is_leaf(is_leaf) {}
};

class BPlusTree
{
private:
    Node *root; // 根节点指针
    int d;      // 阶数
    int hd;     // 分裂时的分割点
    int hf;     // 删除后节点最小键数量

public:
    BPlusTree(int d);

    int get(int key);
    void insert(int k, int v);
    void remove(int k);

private:
    Node *_search(int k);
    void _split(Node *node);
    void _add(Node *parent, int k, Node *l, Node *r);
    void _fix(Node *node);
    void _merge(Node *node);
};

#endif
