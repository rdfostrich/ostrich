#ifndef TPFPATCH_STORE_PATCH_TREE_KEY_COMPARATOR_H
#define TPFPATCH_STORE_PATCH_TREE_KEY_COMPARATOR_H

#include <kchashdb.h>
#include "patch.h"

using namespace kyotocabinet;

// A PatchTreeKeyComparator can be used in a Kyoto Cabinet TreeDB for ordering by PatchTreeKey.
class PatchTreeKeyComparator : public Comparator {
public:
    int32_t compare(const char* akbuf, size_t aksiz, const char* bkbuf, size_t bksiz);
    int32_t compare(PatchTreeKey key1, PatchTreeKey key2);
};


#endif //TPFPATCH_STORE_PATCH_TREE_KEY_COMPARATOR_H
