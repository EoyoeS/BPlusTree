#include <iostream>
#include "bplus_tree.hpp"

void show_page(BPlusTree &tree)
{
    for (page_num_t i = 0; i < tree.cnt; ++i)
    {
        auto look = tree.readNode(i);
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

int main()
{
    BPlusTree tree("bplus_tree.dat");
    for (int i = 0; i < 5000; i += 2)
    {
        tree.insert(i, i * i);
    }
    for (int i = 0; i < 5000; i += 3)
    {
        tree.remove(i);
    }
    for (int i = 0; i < 5000; ++i)
    {
        value_t v = tree.get(i);
        if (v != -1)
        {
            printf("(%d, %d)  ", i, v);
        }
    }
    printf("\n");
    show_page(tree);
}