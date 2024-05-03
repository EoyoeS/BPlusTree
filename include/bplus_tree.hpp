#ifndef BPLUSTREE_HPP
#define BPLUSTREE_HPP

#include <iostream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <list>
#include <fstream>
#include <memory>

typedef std::int32_t key_t;
typedef std::int64_t value_t;
typedef std::int64_t page_num_t;
typedef std::size_t size_t;

const std::int32_t PAGE_SIZE = 16384;
// const std::int32_t PAGE_SIZE = 8192;
const std::int32_t LOGGING_SIZE = 4096;
const std::int32_t NODE_SIZE = PAGE_SIZE + LOGGING_SIZE;
const std::int32_t K = 64;                            // 将l_pg分为K段，K需被8整除
const std::int32_t F_SIZE = K / 8;                    // f向量的大小
const std::int32_t DELTA_OFFSET = PAGE_SIZE + F_SIZE; // ∆的偏移量
const std::int32_t D = PAGE_SIZE / K;                 // 每段的大小
const std::int32_t T = 2048;                          // 当|∆|>T时，刷新page，最大T=(LOGGING_SIZE-K/8)/D

const std::int32_t MAX_INDEX_NUM = (PAGE_SIZE - sizeof(bool) - sizeof(page_num_t) - sizeof(page_num_t) - sizeof(size_t)) / (sizeof(key_t) + sizeof(page_num_t));
const std::int32_t MAX_DATA_NUM = (PAGE_SIZE - sizeof(bool) - sizeof(page_num_t) - sizeof(page_num_t) - sizeof(size_t)) / (sizeof(key_t) + sizeof(value_t)) + 1;

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

class LRUCache
{
public:
    std::size_t capacity;
    std::list<std::pair<page_num_t, std::unique_ptr<Node>>> lru;
    std::unordered_map<page_num_t, std::list<std::pair<page_num_t, std::unique_ptr<Node>>>::iterator> cache;

    LRUCache(std::size_t capacity) : capacity(capacity) {}

    std::unique_ptr<Node> get(page_num_t key)
    {
        auto it = cache.find(key);
        if (it == cache.end())
        {
            return nullptr;
        }
        std::unique_ptr<Node> value = std::make_unique<Node>(*it->second->second);
        lru.emplace_front(key, std::move(it->second->second));
        lru.erase(it->second);
        cache.erase(key);
        cache.emplace(key, lru.begin());
        return value;
    }

    page_num_t put(page_num_t key, std::unique_ptr<Node> value)
    {
        auto it = cache.find(key);
        page_num_t ret = 0;
        if (it != cache.end())
        {
            lru.erase(it->second);
            cache.erase(key);
        }
        else if (cache.size() == capacity)
        {
            cache.erase(lru.back().first);
            lru.pop_back();
        }
        lru.emplace_front(key, std::move(value));
        cache.emplace(key, lru.begin());
        if (cache.size() == capacity)
        {
            ret = lru.back().first;
        }
        return ret;
    }

    void clear()
    {
        lru.clear();
        cache.clear();
    }
};

class BPlusTree
{
public:
    std::int32_t max_index;  // 阶数：非叶节点存储的子节点的最大数量
    std::int32_t max_data;   // 叶节点最多有d2-1条记录
    std::int32_t mid_indx;   // 分裂时叶节点的分割点
    std::int32_t mid_data;   // 分裂时叶节点的分割点
    std::int32_t half_index; // 删除后非叶节点最小键数量
    std::int32_t half_data;  // 删除后叶节点最小键数量
    page_num_t root;         // 根节点页号
    page_num_t cnt;          // 页号计数
    std::string file_path;   // 文件路径
    std::fstream file;       // 文件流
    // std::unordered_map<page_num_t, std::unique_ptr<Node>> cache; // 缓存
    LRUCache cache = LRUCache(200);
    std::unordered_set<page_num_t> to_write; // 待写入的节点
    double cost_time;

    explicit BPlusTree(const std::string &file_path, bool read = false);

    value_t get(key_t key);
    void insert(key_t k, value_t v);
    void remove(key_t k);
    std::unique_ptr<Node> read_node(page_num_t pos);
    void flush();

private:
    std::unique_ptr<Node> _search(key_t k);
    void _add(page_num_t parent, key_t k, std::unique_ptr<Node> &node, std::unique_ptr<Node> &new_node);
    void _split(std::unique_ptr<Node> &pos);
    void _fix(std::unique_ptr<Node> &node);
    void _merge(std::unique_ptr<Node> &pos);
    void write_node(std::unique_ptr<Node> &node);
    void flush_node(std::unique_ptr<Node> node);
    void write_node_with_logging(std::unique_ptr<Node> &node);
    std::unique_ptr<Node> read_node_with_logging(page_num_t pos);
    void _read();
    void _write();
};

#endif
