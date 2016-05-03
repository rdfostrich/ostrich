#ifndef TPFPATCH_STORE_PATCH_TREE_KEY_COMPARATOR_H
#define TPFPATCH_STORE_PATCH_TREE_KEY_COMPARATOR_H

#include <kchashdb.h>
#include "patch.h"

using namespace kyotocabinet;

typedef std::function<int32_t(PatchTreeKey*, PatchTreeKey*)> comp;

auto comp_s = [] (PatchTreeKey* e1, PatchTreeKey* e2) { return e1->get_subject  ().compare(e2->get_subject  ()); };
auto comp_p = [] (PatchTreeKey* e1, PatchTreeKey* e2) { return e1->get_predicate().compare(e2->get_predicate()); };
auto comp_o = [] (PatchTreeKey* e1, PatchTreeKey* e2) { return e1->get_object   ().compare(e2->get_object   ()); };

// A PatchTreeKeyComparator can be used in a Kyoto Cabinet TreeDB for ordering by PatchTreeKey.
class PatchTreeKeyComparator : public Comparator {
protected:
    comp compare_1;
    comp compare_2;
    comp compare_3;
public:
    static PatchTreeKeyComparator comparator_spo;
    PatchTreeKeyComparator(comp compare_1, comp compare_2, comp compare_3);
    int32_t compare(const char* akbuf, size_t aksiz, const char* bkbuf, size_t bksiz);
    int32_t compare(PatchTreeKey key1, PatchTreeKey key2);
};

#endif //TPFPATCH_STORE_PATCH_TREE_KEY_COMPARATOR_H
