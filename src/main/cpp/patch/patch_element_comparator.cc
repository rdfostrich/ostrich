#include "patch_element_comparator.h"

PatchElementComparator::PatchElementComparator(PatchTreeKeyComparator* patchtreekey_comparator) :
        patchtreekey_comparator(patchtreekey_comparator) {}

std::function<int(const PatchElement &lhs, const PatchElement &rhs)> PatchElementComparator::get() {
    PatchTreeKeyComparator* comp = patchtreekey_comparator;
    return [comp](const PatchElement &lhs, const PatchElement &rhs) {
        return comp->compare(lhs.get_triple(), rhs.get_triple()) < 0 ||
               (lhs.get_triple() == rhs.get_triple() && !lhs.is_addition() && rhs.is_addition());
    };
}
