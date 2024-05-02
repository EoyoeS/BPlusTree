#include <cstring>
#include <chrono>

#include "bplus_tree.hpp"

void to_buffer(char *buffer, std::unique_ptr<Node> &node)
{
    size_t offset = 0;
    memcpy(buffer + offset, (char *)&node->is_leaf, sizeof(bool));
    offset += sizeof(bool);
    memcpy(buffer + offset, (char *)&node->pos, sizeof(page_num_t));
    offset += sizeof(page_num_t);
    memcpy(buffer + offset, (char *)&node->parent, sizeof(page_num_t));
    offset += sizeof(page_num_t);
    size_t key_num = node->keys.size();
    memcpy(buffer + offset, (char *)&key_num, sizeof(size_t));
    offset += sizeof(size_t);
    size_t offset_ = offset;
    for (key_t key : node->keys)
    {
        memcpy(buffer + offset, (char *)&key, sizeof(key_t));
        offset += sizeof(key_t);
    }
    if (!node->is_leaf)
    {
        offset_ += MAX_INDEX_NUM * sizeof(key_t);
        for (value_t child : node->children)
        {
            memcpy(buffer + offset_, (char *)&child, sizeof(value_t));
            offset_ += sizeof(value_t);
        }
    }
    else
    {
        offset_ += MAX_DATA_NUM * sizeof(key_t);
        for (value_t value : node->values)
        {
            memcpy(buffer + offset_, (char *)&value, sizeof(value_t));
            offset_ += sizeof(value_t);
        }
    }
}

void BPlusTree::write_node(std::unique_ptr<Node> &node)
{
    cache[node->pos] = std::make_unique<Node>(*node);
    to_write.insert(node->pos);
}

void BPlusTree::write_node_with_logging(std::unique_ptr<Node> &node)
{
    char buffer_in_mem[PAGE_SIZE];
    char buffer_page[PAGE_SIZE];
    char buffer_logging[LOGGING_SIZE];
    memset(buffer_in_mem, 0, PAGE_SIZE);
    memset(buffer_logging, 0, LOGGING_SIZE);
    to_buffer(buffer_in_mem, node);
    file.seekg(0, std::ios::end);
    if (node->pos * NODE_SIZE >= file.tellg())
    {
        file.seekp(node->pos * NODE_SIZE);
        file.write(buffer_in_mem, PAGE_SIZE);
        file.write(buffer_logging, LOGGING_SIZE);
        auto end = std::chrono::high_resolution_clock::now();
        return;
    }
    file.seekg(node->pos * NODE_SIZE);
    file.read(buffer_page, PAGE_SIZE);
    for (int i = 0, t = 0, s = 0; i < K; ++i, s += D)
    {
        if (memcmp(buffer_page + s, buffer_in_mem + s, D) != 0)
        {
            if (t == T)
            {
                file.seekp(node->pos * NODE_SIZE);
                file.write(buffer_in_mem, PAGE_SIZE);
                memset(buffer_logging, 0, LOGGING_SIZE);
                file.write(buffer_logging, LOGGING_SIZE);
                auto end = std::chrono::high_resolution_clock::now();
                return;
            }
            memcpy(buffer_logging + F_SIZE + t, buffer_in_mem + s, D);
            t += D;
            buffer_logging[i / 8] |= 1 << (i % 8);
        }
    }
    file.seekp(node->pos * NODE_SIZE + PAGE_SIZE);
    file.write(buffer_logging, LOGGING_SIZE);
    auto end = std::chrono::high_resolution_clock::now();
}

std::unique_ptr<Node> to_node(char *buffer)
{
    size_t offset = 0;
    bool is_leaf;
    page_num_t pos, parent;
    size_t key_num;
    memcpy((char *)&is_leaf, buffer + offset, sizeof(bool));
    offset += sizeof(bool);
    memcpy((char *)&pos, buffer + offset, sizeof(page_num_t));
    offset += sizeof(page_num_t);
    memcpy((char *)&parent, buffer + offset, sizeof(page_num_t));
    offset += sizeof(page_num_t);
    std::unique_ptr<Node> node(new Node(parent, is_leaf, pos));
    memcpy((char *)&key_num, buffer + offset, sizeof(size_t));
    offset += sizeof(size_t);
    size_t offset_ = offset;
    node->keys.resize(key_num);
    for (int i = 0; i < key_num; ++i)
    {
        memcpy((char *)&node->keys[i], buffer + offset, sizeof(key_t));
        offset += sizeof(key_t);
    }
    if (!is_leaf)
    {
        offset_ += MAX_INDEX_NUM * sizeof(key_t);
        size_t child_num = key_num + 1;
        node->children.resize(child_num);
        for (int i = 0; i < node->children.size(); ++i)
        {
            memcpy((char *)&node->children[i], buffer + offset_, sizeof(value_t));
            offset_ += sizeof(value_t);
        }
    }
    else
    {
        offset_ += MAX_DATA_NUM * sizeof(key_t);
        node->values.resize(key_num);
        for (int i = 0; i < key_num; ++i)
        {
            memcpy((char *)&node->values[i], buffer + offset_, sizeof(value_t));
            offset_ += sizeof(value_t);
        }
    }
    return node;
}

std::unique_ptr<Node> BPlusTree::read_node(page_num_t pos)
{
    if (cache.find(pos) != cache.end())
    {
        return std::make_unique<Node>(*cache[pos]);
    }
    char buffer[PAGE_SIZE];
    file.seekg(pos * PAGE_SIZE);
    file.read(buffer, PAGE_SIZE);
    cache[pos] = to_node(buffer);
    // cache[pos] = read_node_with_logging(pos);
    return std::make_unique<Node>(*cache[pos]);
}

std::unique_ptr<Node> BPlusTree::read_node_with_logging(page_num_t pos)
{
    char buffer[NODE_SIZE];
    file.seekg(pos * NODE_SIZE);
    file.read(buffer, NODE_SIZE);
    for (int i = 0, s = 0, t = 0; i < K; ++i, s += D)
    {
        if (buffer[PAGE_SIZE + i / 8] & 1)
        {
            memcpy(buffer + s, buffer + DELTA_OFFSET + t, D);
            t += D;
        }
        buffer[PAGE_SIZE + i / 8] >>= 1;
    }
    return to_node(buffer);
}

void BPlusTree::_write()
{
    char buffer[PAGE_SIZE];
    size_t offset = 0;
    memcpy(buffer + offset, (char *)&max_index, sizeof(int32_t));
    offset += sizeof(int32_t);
    memcpy(buffer + offset, (char *)&max_data, sizeof(int32_t));
    offset += sizeof(int32_t);
    memcpy(buffer + offset, (char *)&root, sizeof(page_num_t));
    offset += sizeof(page_num_t);
    memcpy(buffer + offset, (char *)&cnt, sizeof(page_num_t));
    offset += sizeof(page_num_t);
    file.seekp(0);
    file.write(buffer, PAGE_SIZE);
    file.flush();
}

void BPlusTree::_read()
{
    char buffer[PAGE_SIZE];
    file.seekg(0);
    file.read(buffer, PAGE_SIZE);
    size_t offset = 0;
    memcpy((char *)&this->max_index, buffer + offset, sizeof(int32_t));
    offset += sizeof(int32_t);
    memcpy((char *)&this->max_data, buffer + offset, sizeof(int32_t));
    offset += sizeof(int32_t);
    memcpy((char *)&this->root, buffer + offset, sizeof(page_num_t));
    offset += sizeof(page_num_t);
    memcpy((char *)&this->cnt, buffer + offset, sizeof(page_num_t));
    offset += sizeof(page_num_t);
}

void BPlusTree::flush_node(std::unique_ptr<Node> node)
{
    char buffer_in_mem[PAGE_SIZE];
    to_buffer(buffer_in_mem, node);
    file.seekp(node->pos * PAGE_SIZE);
    file.write(buffer_in_mem, PAGE_SIZE);
    // write_node_with_logging(node);
}

void BPlusTree::flush()
{
    for (page_num_t pos : to_write)
    {
        flush_node(std::move(cache[pos]));
        cache.erase(pos);
    }
    to_write.clear();
    cache.clear();
    auto start = std::chrono::high_resolution_clock::now();
    file.flush();
    _write();
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = end - start;
    cost_time += duration.count();
}