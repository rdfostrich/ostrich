#ifndef TPFPATCH_STORE_PATCH_TREE_H
#define TPFPATCH_STORE_PATCH_TREE_H

#include <string>
#include <kchashdb.h>
#include "patch_tree_iterator.h"
#include "patch.h"
#include "patch.h"
#include "patch_tree_key_comparator.h"
#include "positioned_triple_iterator.h"
#include "triple_store.h"

using namespace std;
using namespace kyotocabinet;

// A PatchTree can store Patches which are persisted to a file
class PatchTree {
private:
    TripleStore* tripleStore;
    PatchTreeKeyComparator* keyComparator;
public:
    PatchTree(string file_name);
    ~PatchTree();
    /**
     * Append the given patch elements to the tree with given patch id.
     * This can OVERWRITE existing elements without a warning.
     * @param patch The patch elements
     * @param patch_id The id of the patch
     * @note If an error occurs, some elements might have already been added.
     * If you want to change this behaviour, you'll have to first check if the patch elements are really new.
     */
    void append_unsafe(Patch patch, int patch_id);
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
    /**
     * Reconstruct a patch based on the given patch id.
     * It will loop over the tree and rebuild the patch.
     * @param patch_id The patch id
     * @param ignore_local_changes If local changes should be ignored when reconstructing the patch, false by default.
     * @return The reconstructed patch
     */
    Patch reconstruct_patch(int patch_id, bool ignore_local_changes = false);
    /**
     * Get an iterator starting from the given key.
     * @param key The key to start from
     * @return The iterator that will loop over the tree from the given key.
     */
    PatchTreeIterator iterator(PatchTreeKey* key);
    /**
     * Get an iterator starting from the start of the tree and only emitting the elements in the given patch.
     * @param patch_id The patch id to filter by, this includes all patches before this one.
     * @param exact If only patches exactly matching the given id should be returned,
     *              otherwise all patches with an id <= patch_id will be returned.
     * @return The iterator that will loop over the tree for the given patch.
     */
    PatchTreeIterator iterator(int patch_id, bool exact);
    /**
     * Get an iterator starting from the given key and only emitting the elements in the given patch.
     * @param key The key to start from
     * @param patch_id The patch id to filter by, this includes all patches before this one.
     * @param exact If only patches exactly matching the given id should be returned,
     *              otherwise all patches with an id <= patch_id will be returned.
     * @return The iterator that will loop over the tree for the given patch.
     */
    PatchTreeIterator iterator(PatchTreeKey* key, int patch_id, bool exact);
    /**
     * Get the number of deletions.
     * @param triple_pattern The triple pattern to match by.
     * @param patch_id The patch id to get the deletions for, this patch id must exist within the tree!
     * @return The amount of deletions matching the given triple pattern for the given patch id.
     */
    PatchPosition deletion_count(Triple triple_pattern, int patch_id);
    /**
     * Get an iterator that loops over all deletions starting from a given triple and only matching the
     * given triple pattern.
     * @param offset The triple to start from.
     * @param patch_id The patch id to filter by, this includes all patches before this one.
     * @param triple_pattern Only triples that match the given pattern will be returned in the iterator.
     * @return The iterator that will loop over the tree for the given patch.
     */
    PositionedTripleIterator deletion_iterator_from(Triple offset, int patch_id, Triple triple_pattern);
    /**
     * Get an iterator that loops over all additions starting with a given offset and only matching the
     * given triple pattern.
     * @param offset The number of addition triples to skip.
     * @param patch_id The patch id to filter by, this includes all patches before this one.
     * @param triple_pattern Only triples that match the given pattern will be returned in the iterator.
     * @return The iterator that will loop over the tree for the given patch.
     */
    PositionedTripleIterator addition_iterator_from(int offset, int patch_id, Triple triple_pattern);
};

#endif //TPFPATCH_STORE_PATCH_TREE_H
