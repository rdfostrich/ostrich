#include <iostream>

#include "patch_tree_key_comparator.h"

PatchTreeKeyComparator PatchTreeKeyComparator::comparator_spo(comp_s, comp_p, comp_o, dict);

PatchTreeKeyComparator::PatchTreeKeyComparator(comp compare_1, comp compare_2, comp compare_3, Dictionary* dict)
        : compare_1(compare_1), compare_2(compare_2), compare_3(compare_3), dict(dict) {}

// TODO: use dictionary
int32_t PatchTreeKeyComparator::compare(const char* akbuf, size_t aksiz, const char* bkbuf, size_t bksiz) {
    PatchTreeKey element1;
    PatchTreeKey element2;
    element1.deserialize(akbuf, aksiz);
    element2.deserialize(bkbuf, bksiz);
    int comp_1 = compare_1(element1, element2);
    if(!comp_1) {
        int comp_2 = compare_2(element1, element2);
        if(!comp_2) {
            return compare_3(element1, element2);
        }
        return comp_2;
    }
    return comp_1;
};

int32_t PatchTreeKeyComparator::compare(const PatchTreeKey& element1, const PatchTreeKey& element2) {
    int comp_1 = compare_1(element1, element2);
    if(!comp_1) {
        int comp_2 = compare_2(element1, element2);
        if(!comp_2) {
            return compare_3(element1, element2);
        }
        return comp_2;
    }
    return comp_1;
}
