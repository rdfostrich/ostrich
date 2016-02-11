#ifndef TPFPATCH_STORE_PATCH_TREE_KEY_COMPARATOR_H
#define TPFPATCH_STORE_PATCH_TREE_KEY_COMPARATOR_H

#include <kchashdb.h>
#include "patch.h"

using namespace kyotocabinet;

class PatchTreeKeyComparator : public Comparator {
public:
    int32_t compare(const char* akbuf, size_t aksiz, const char* bkbuf, size_t bksiz);
    int32_t compare(PatchTreeKey key1, PatchTreeKey key2);
};


#endif //TPFPATCH_STORE_PATCH_TREE_KEY_COMPARATOR_H
