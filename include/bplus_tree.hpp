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
    Node *root;    // 根节点指针
    int d;         // 阶数：非叶节点存储的子节点的最大数量
    int d2;        // 叶节点最多有d2-1条记录
    int hd;        // 分裂时叶节点的分割点
    int hd2;       // 分裂时叶节点的分割点
    int hf;        // 删除后非叶节点最小键数量
    int hf2;       // 删除后叶节点最小键数量

public:
    BPlusTree(int d, int d2);

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
