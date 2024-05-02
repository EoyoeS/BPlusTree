#include <cstring>
#include "bplus_tree.hpp"

const std::string PATH = "/home/wuhepei/biyeueji/mount_sfdv/rocksdb_tmp";


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
    BPlusTree tree(PATH, true);
    for (int i = 0; i < 1000000; i += 2)
    {
        tree.insert(i, i << 1);
        // if (tree.cache.size() > 1000)
        {
            tree.flush();
        }
    }
    tree.flush();
    auto node = tree.read_node(tree.root);
    // for (int i = 0; i < 100000; i += 3)
    // {
    //     tree.remove(i);
    // }
    // tree.file.seekp(PAGE_SIZE);
    // tree.file.write(tree._buffer, tree.cnt * PAGE_SIZE);
    // tree.file.flush();
    printf("cost %.2f ms\n", tree.cost_time);
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