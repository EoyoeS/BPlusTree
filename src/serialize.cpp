#include <cstring>

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
    for (key_t key : node->keys)
    {
        memcpy(buffer + offset, (char *)&key, sizeof(key_t));
        offset += sizeof(key_t);
    }
    if (!node->is_leaf)
    {
        for (value_t child : node->children)
        {
            memcpy(buffer + offset, (char *)&child, sizeof(value_t));
            offset += sizeof(value_t);
        }
    }
    else
    {
        for (value_t value : node->values)
        {
            memcpy(buffer + offset, (char *)&value, sizeof(value_t));
            offset += sizeof(value_t);
        }
    }
}

void BPlusTree::write_node(std::unique_ptr<Node> &node)
{
    // char buffer_in_mem[PAGE_SIZE];
    // to_buffer(buffer_in_mem, node);
    // file.seekp(node->pos * PAGE_SIZE);
    // file.write(buffer_in_mem, PAGE_SIZE);
    // file.flush();
    write_node_with_logging(node);
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
    if (node->pos * NODE_SIZE == file.tellg())
    {
        file.seekp(0, std::ios::end);
        file.write(buffer_in_mem, PAGE_SIZE);
        file.write(buffer_logging, LOGGING_SIZE);
        file.flush();
        return;
    }
    file.seekg(node->pos * NODE_SIZE);
    file.read(buffer_page, PAGE_SIZE);
    for (int i = 0, j = 0, t = 0, s = 0; i < K; ++i, s += D)
    {
        if (memcmp(buffer_page + s, buffer_in_mem + s, D) != 0)
        {
            if (j == T)
            {
                file.seekp(node->pos * NODE_SIZE);
                file.write(buffer_in_mem, PAGE_SIZE);
                memset(buffer_logging, 0, LOGGING_SIZE);
                file.write(buffer_logging, LOGGING_SIZE);
                file.flush();
                return;
            }
            memcpy(buffer_logging + F_SIZE + t, buffer_in_mem + s, D);
            t += D;
            ++j;
            buffer_logging[i / 8] |= 1 << (i % 8);
        }
    }
    file.seekp(node->pos * NODE_SIZE + PAGE_SIZE);
    file.write(buffer_logging, LOGGING_SIZE);
    file.flush();
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
    node->keys.resize(key_num);
    for (int i = 0; i < key_num; ++i)
    {
        memcpy((char *)&node->keys[i], buffer + offset, sizeof(key_t));
        offset += sizeof(key_t);
    }
    if (!node->is_leaf)
    {
        size_t child_num = key_num + 1;
        node->children.resize(child_num);
        for (int i = 0; i < node->children.size(); ++i)
        {
            memcpy((char *)&node->children[i], buffer + offset, sizeof(value_t));
            offset += sizeof(value_t);
        }
    }
    else
    {
        node->values.resize(key_num);
        for (int i = 0; i < key_num; ++i)
        {
            memcpy((char *)&node->values[i], buffer + offset, sizeof(value_t));
            offset += sizeof(value_t);
        }
    }
    return node;
}

std::unique_ptr<Node> BPlusTree::read_node(page_num_t pos)
{
    // char buffer[PAGE_SIZE];
    // file.seekg(pos * PAGE_SIZE);
    // file.read(buffer, PAGE_SIZE);
    // return to_node(buffer);
    return read_node_with_logging(pos);
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
    char buffer[NODE_SIZE];
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
    file.write(buffer, NODE_SIZE);
    file.flush();
}

void BPlusTree::_read()
{
    char buffer[NODE_SIZE];
    file.seekg(0);
    file.read(buffer, NODE_SIZE);
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