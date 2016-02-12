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
    /**
     * Append the given patch elements to the tree with given patch id
     * @param The patch elements
     * @param The id of the patch
     * @return -1 if one of the patch elements were already present in the tree, 0 otherwise
     * @note If an error occurs, some elements might have already been added.
     * If you want to change this behaviour, you'll have to first check if the patch elements are really new.
     */
    int append(Patch patch, int patch_id);
    Patch reconstruct_patch(int patch_id);
    PatchTreeIterator iterator(PatchTreeKey* key);
    PatchTreeIterator iterator(int patch_id);
};

#endif //TPFPATCH_STORE_PATCH_TREE_H
