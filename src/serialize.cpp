#include <cstring>

#include "bplus_tree.hpp"

void BPlusTree::writeNode(std::unique_ptr<Node> &node)
{
    char *buffer = new char[PAGE_SIZE];
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
    file.seekp(node->pos * PAGE_SIZE);
    file.write(buffer, PAGE_SIZE);
    file.flush();
    delete[] buffer;
}

std::unique_ptr<Node> BPlusTree::readNode(page_num_t pos)
{
    char *buffer = new char[PAGE_SIZE];
    file.seekg(pos * PAGE_SIZE);
    file.read(buffer, PAGE_SIZE);
    size_t offset = 0;
    bool is_leaf;
    page_num_t parent;
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
    delete[] buffer;
    return node;
}

void BPlusTree::_write()
{
    char *buffer = new char[PAGE_SIZE];
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
    delete[] buffer;
}

void BPlusTree::_read()
{
    char *buffer = new char[PAGE_SIZE];
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
    delete[] buffer;
}