#ifndef TPFPATCH_STORE_PATCH_TREE_H
#define TPFPATCH_STORE_PATCH_TREE_H

#include <string>
#include <kchashdb.h>
#include "patch_tree_iterator.h"
#include "patch.h"
#include "patch.h"
#include "patch_tree_key_comparator.h"

using namespace std;
using namespace kyotocabinet;

class PatchTree {
private:
    TreeDB db;
    PatchTreeKeyComparator* keyComparator;
public:
    PatchTree(string file_name);
    ~PatchTree();
    Patch reconstruct_patch(int patch_id);
    int append(Patch patch, int patch_id);
    PatchTreeIterator iterator(PatchTreeKey* key);
    PatchTreeIterator iterator(int patch_id);
};

#endif //TPFPATCH_STORE_PATCH_TREE_H
