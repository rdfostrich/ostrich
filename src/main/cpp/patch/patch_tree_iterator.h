#ifndef TPFPATCH_STORE_PATCH_TREE_ITERATOR_H
#define TPFPATCH_STORE_PATCH_TREE_ITERATOR_H

#include <string>
#include <kchashdb.h>
#include "patch.h"
#include "patch_tree_value.h"
#include "patch_tree_addition_value.h"

using namespace std;
using namespace kyotocabinet;

class PatchTreeIterator {
private:
    DB::Cursor* cursor;

    bool is_patch_id_filter;
    bool is_patch_id_filter_exact;
    int patch_id_filter;

    bool is_addition_filter;
    int addition_filter;

    bool is_triple_pattern_filter;
    Triple triple_pattern_filter;

    bool is_filter_local_changes;

    bool deletion_tree;

    bool reverse;
public:
    PatchTreeIterator(DB::Cursor* cursor);
    ~PatchTreeIterator();
    /**
     * Set the patch id to filter by
     * @param patch_id The patch id to filter by
     * @param exact If only patches exactly matching the given id should be returned,
     *              otherwise all patches with an id <= patch_id will be returned.
     */
    void set_patch_filter(int patch_id, bool exact);
    /**
     * Set the patch id to filter by
     * @param addition True if only additions should be returned, false for only deletions.
     */
    void set_type_filter(bool addition);
    /**
     * Set the triple pattern to filter by
     * @param triple_pattern The triple pattern that will match all results from this iterator.
     */
    void set_triple_pattern_filter(Triple triple_pattern);
    /**
     * Indicate that this iterator should ignore local changes.
     */
    void set_filter_local_changes(bool filter_local_changes);
    /**
     * Indicate if this iterator is iterating over a deletion tree or not.
     */
    void set_deletion_tree(bool deletion_tree);
    /**
     * @return If this iterator is iterating over a deletion tree.
     */
    bool is_deletion_tree();
    /**
     * Indicate if this iterator should step backwards.
     * @param reverse If it should go reverse.
     */
    void set_reverse(bool reverse);
    /**
     * @return If this iterator goes in reverse.
     */
    bool is_reverse();
    /**
     * Point to the next element
     * Can only be called if iterating over a deletion tree.
     * @param key The key the iterator is currently pointing at.
     * @param value The value the iterator is currently pointing at.
     * @param silent_step If the cursor doesn't need to be moved.
     * @return If this next element exists, otherwise the key and value will be invalid and should be ignored.
     */
    bool next(PatchTreeKey* key, PatchTreeValue* value, bool silent_step = false);
    /**
     * Point to the next element
     * Can only be called if iterating over an addition tree.
     * @param key The key the iterator is currently pointing at.
     * @param value The value the iterator is currently pointing at.
     * @return If this next element exists, otherwise the key and value will be invalid and should be ignored.
     */
    bool next(PatchTreeKey* key, PatchTreeAdditionValue* value);
    /**
     * Point to the next element
     * Can only be called for both an addition tree and deletion tree.
     * @param key The key the iterator is currently pointing at.
     * @param value The value the iterator is currently pointing at.
     * @return If this next element exists, otherwise the key and value will be invalid and should be ignored.
     */
    bool next_addition(PatchTreeKey* key, PatchTreeAdditionValue* value);
};


#endif //TPFPATCH_STORE_PATCH_TREE_ITERATOR_H
