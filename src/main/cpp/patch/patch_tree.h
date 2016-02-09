#ifndef TPFPATCH_STORE_PATCH_TREE_H
#define TPFPATCH_STORE_PATCH_TREE_H

#include <string>
#include <kchashdb.h>
#include "patch_tree_iterator.h"
#include "patch.h"

using namespace std;
using namespace kyotocabinet;

class PatchTree {
private:
    TreeDB db;
public:
    PatchTree(string file_name);
    ~PatchTree();
    void append(PatchElements* patch);
    PatchTreeIterator iterator(PatchTreeKey* key);
};

#endif //TPFPATCH_STORE_PATCH_TREE_H
