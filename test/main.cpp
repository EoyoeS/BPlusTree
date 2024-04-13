#include <iostream>
#include "bplus_tree.hpp"

int main()
{
    BPlusTree tree(5);
    for (int i = 0; i < 50; i += 2)
    {
        tree.insert(i, i * i);
    }
    for (int i = 0; i < 50; i += 3)
    {
        tree.remove(i);
    }
    for (int i = 0; i < 50; ++i)
    {
        int v = tree.get(i);
        if (v != -1)
        {
            std::cout << i << ' ' << v << std::endl;
        }
    }
}