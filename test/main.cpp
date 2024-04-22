#include <cstring>
#include <chrono>
#include "bplus_tree.hpp"

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
                printf(" %d", look->values[j]);
            }
            printf("\n\n");
        }
    }
}

int main(int argc, char *argv[])
{
    BPlusTree tree("/dev/sfdv0n1", argc == 2 && strcmp(argv[1], "reset") == 0);
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 5000; i += 2)
    {
        tree.insert(i, i * i);
    }
    for (int i = 0; i < 5000; i += 3)
    {
        tree.remove(i);
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = end - start;
    auto k = duration.count();
    printf("cost %.2f ms\n", k);
    // for (int i = 0; i < 5000; ++i)
    // {
    //     value_t v = tree.get(i);
    //     if (v != -1)
    //     {
    //         printf("(%d, %d)  ", i, v);
    //     }
    // }
    // printf("\n");
    // show_page(tree);
}