#include <iostream>

#include "patch_tree_key_comparator.h"

PatchTreeKeyComparator::PatchTreeKeyComparator(comp compare_1, comp compare_2, comp compare_3)
        : compare_1(compare_1), compare_2(compare_2), compare_3(compare_3) {}

// TODO: use dictionary
int32_t PatchTreeKeyComparator::compare(const char* akbuf, size_t aksiz, const char* bkbuf, size_t bksiz) {
    PatchTreeKey element1;
    PatchTreeKey element2;
    element1.deserialize(akbuf, aksiz);
    element2.deserialize(bkbuf, bksiz);
    int comp_1 = compare_1(&element1, &element2);
    if(!comp_1) {
        int comp_2 = compare_2(&element1, &element2);
        if(!comp_2) {
            return compare_3(&element1, &element2);
        }
        return comp_2;
    }
    return comp_1;
};

int32_t PatchTreeKeyComparator::compare(PatchTreeKey key1, PatchTreeKey key2) {
    size_t key1_size, key2_size;
    const char* key1_data = key1.serialize(&key1_size);
    const char* key2_data = key2.serialize(&key2_size);
    int ret = compare(key1_data, key1_size, key2_data, key2_size);
    free((char*) key1_data);
    free((char*) key2_data);
    return ret;
}
