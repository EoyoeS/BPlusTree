#include <cstring>
#include "bplus_tree.hpp"
#include <chrono>
#include <random>
#include <algorithm>

const std::string PATH = "/home/wuhepei/biyeueji/mount_sfdv/bplustree_tmp";
// const std::string PATH = "bplustree_tmp";

void show_page(BPlusTree &tree)
{
    printf("root: %ld\n", tree.root);
    for (page_num_t i = 1; i < tree.cnt; ++i)
    {
        auto look = tree.read_node(i);
        printf("page %ld: pos %ld, parent %ld, type %d\n", i, look->pos, look->parent, look->is_leaf);
        printf("keys:");
        for (size_t j = 0; j < look->keys.size(); ++j)
        {
            printf(" %d", look->keys[j]);
        }
        printf("\n");
        if (!look->is_leaf)
        {
            printf("children:");
            for (size_t j = 0; j < look->children.size(); ++j)
            {
                printf(" %ld", look->children[j]);
            }
            printf("\n\n");
        }
        else
        {
            printf("values:");
            for (size_t j = 0; j < look->values.size(); ++j)
            {
                printf(" %ld", look->values[j]);
            }
            printf("\n\n");
        }
    }
}

int main(int argc, char *argv[])
{
    int num = 500000;
    std::vector<int> elem;
    elem.reserve(num);
    for (int i = 0; i < num; ++i)
    {
        elem.push_back(i);
    }
    std::mt19937 rng(42);
    // std::shuffle(elem.begin(), elem.end(), rng);
    BPlusTree tree(PATH, true);
    auto start = std::chrono::high_resolution_clock::now();
    for (auto &i : elem)
    {
        tree.insert(i, i << 1);
    }
    // for (int i = 0; i < 1000000; i += 2)
    // {
    //     tree.insert(i, i << 1);
    //     // if (tree.cache.size() > 1000)
    //     // {
    //     //     tree.flush();
    //     // }
    // }
    tree.flush();
    // tree.flush();
    // auto node = tree.read_node(tree.root);
    // for (int i = 0; i < 1000000; i += 3)
    // {
    //     tree.remove(i);
    // }
    // tree.flush();
    // tree.file.seekp(PAGE_SIZE);
    // tree.file.write(tree._buffer, tree.cnt * PAGE_SIZE);
    // tree.file.flush();
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = end - start;
    // printf("cost %.2f ms\n", tree.cost_time);
    printf("cost %.2f ms\n", duration.count());
    printf("num: %ld\n", tree.cnt);
    // printf("num of cache: %ld\n", tree.cache.size());
    for (auto &k : tree.cache.lru)
    {
        printf("%ld  ", k.first);
    }
    printf("\n");

    // for (int i = 97918; i < 98099; ++i)
    // {
    //     value_t v = tree.get(i);
    //     if (v != -1)
    //     {
    //         printf("(%d, %ld)  ", i, v);
    //     }
    // }
    // printf("\n");
    // show_page(tree);
}