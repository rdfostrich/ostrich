#ifndef TPFPATCH_STORE_PATCH_ELEMENT_COMPARATOR_H
#define TPFPATCH_STORE_PATCH_ELEMENT_COMPARATOR_H

#include "patch_tree_key_comparator.h"
#include "patch_element.h"

class PatchElementComparator{
protected:
    PatchTreeKeyComparator* patchtreekey_comparator;
public:
    PatchElementComparator(PatchTreeKeyComparator* patchtreekey_comparator);
    std::function<int(const PatchElement &lhs, const PatchElement &rhs)> get();
};

#endif //TPFPATCH_STORE_PATCH_ELEMENT_COMPARATOR_H
