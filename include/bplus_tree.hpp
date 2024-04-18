#ifndef BPLUSTREE_HPP
#define BPLUSTREE_HPP

#include <iostream>
#include <vector>
#include <fstream>
#include <memory>

#define PAGE_SIZE 4096
#define MAX_INDEX_NUM 100
#define MAX_DATA_NUM 100

typedef std::int32_t key_t;
typedef std::int32_t value_t;
typedef std::int64_t page_num_t;
typedef std::size_t size_t;

// 节点结构体
struct Node
{
    std::vector<key_t> keys;          // 键值列表
    std::vector<value_t> values;      // 值列表
    std::vector<page_num_t> children; // 子节点
    bool is_leaf;                     // 是否为叶节点
    page_num_t pos;                   // 页号
    page_num_t parent;                // 父节点页号

    Node(page_num_t parent, bool is_leaf, page_num_t pos)
    {
        this->is_leaf = is_leaf;
        this->keys = std::vector<key_t>();
        this->values = std::vector<value_t>();
        this->children = std::vector<page_num_t>();
        this->pos = pos;
        this->parent = parent;
    }
};

class BPlusTree
{
public:
    int d;             // 阶数：非叶节点存储的子节点的最大数量
    int d2;            // 叶节点最多有d2-1条记录
    int hd;            // 分裂时叶节点的分割点
    int hd2;           // 分裂时叶节点的分割点
    int hf;            // 删除后非叶节点最小键数量
    int hf2;           // 删除后叶节点最小键数量
    page_num_t root;   // 根节点页号
    page_num_t cnt;    // 页号计数
    std::fstream file; // 文件流

    explicit BPlusTree(const std::string &file_path);

    value_t get(key_t key);
    void insert(key_t k, value_t v);
    void remove(key_t k);
    std::unique_ptr<Node> readNode(page_num_t pos);

private:
    std::unique_ptr<Node> _search(key_t k);
    void _add(page_num_t parent, key_t k, std::unique_ptr<Node> &node, std::unique_ptr<Node> &new_node);
    void _split(std::unique_ptr<Node> &pos);
    void _fix(std::unique_ptr<Node> &node);
    void _merge(std::unique_ptr<Node> &pos);
    void writeNode(std::unique_ptr<Node> &node);
};
#endif
