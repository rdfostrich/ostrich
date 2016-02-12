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
     * Append the given patch elements to the tree with given patch id.
     * @param patch The patch elements
     * @param patch_id The id of the patch
     * @return -1 if one of the patch elements were already present in the tree, 0 otherwise
     * @note If an error occurs, some elements might have already been added.
     * If you want to change this behaviour, you'll have to first check if the patch elements are really new.
     */
    int append_unsafe(Patch patch, int patch_id);
    /**
     * Append the given patch elements to the tree with given patch id.
     * This safe append will first check if the patch is completely new, only then it will add the data
     * @param patch The patch elements
     * @param patch_id The id of the patch
     * @return If the patch was added, otherwise the patch was not completely new.
     */
    bool append(Patch patch, int patch_id);
    /**
     * Check if the given patch element is present in the tree.
     * @param patch_element The patch element to look for
     * @param patch_id The id of the patch to look for
     * @param ignore_type If the element type (addition/deletion) should be ignored.
     * @return If the patch is present in the tree.
     */
    bool contains(PatchElement patch_element, int patch_id, bool ignore_type);
    Patch reconstruct_patch(int patch_id);
    PatchTreeIterator iterator(PatchTreeKey* key);
    PatchTreeIterator iterator(int patch_id);
};

#endif //TPFPATCH_STORE_PATCH_TREE_H
