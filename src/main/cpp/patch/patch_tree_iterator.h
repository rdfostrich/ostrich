#ifndef TPFPATCH_STORE_PATCH_TREE_ITERATOR_H
#define TPFPATCH_STORE_PATCH_TREE_ITERATOR_H

#include <string>
#include <kchashdb.h>
#include "patch.h"
#include "patch_tree_value.h"
#include "triple_comparator.h"


template <class DV>
class PatchTreeIteratorBase {
private:
    kyotocabinet::DB::Cursor* cursor_deletions;
    kyotocabinet::DB::Cursor* cursor_additions;
    PatchTreeKeyComparator* comparator;

    bool is_patch_id_filter;
    bool is_patch_id_filter_exact;
    int patch_id_filter;

    bool is_triple_pattern_filter;
    Triple triple_pattern_filter;

    bool is_filter_local_changes;

    bool reverse;

    bool has_temp_key_deletion;
    bool has_temp_key_addition;
    PatchTreeKey* temp_key_deletion;
    PatchTreeKey* temp_key_addition;

    bool can_early_break = true;
    bool squash_equal_addition_deletion = false;
public:
    PatchTreeIteratorBase(kyotocabinet::DB::Cursor* cursor_deletions, kyotocabinet::DB::Cursor* cursor_additions, PatchTreeKeyComparator* comparator);
    ~PatchTreeIteratorBase();
    /**
     * Set the patch id to filter by
     * @param patch_id The patch id to filter by
     * @param exact If only patches exactly matching the given id should be returned,
     *              otherwise all patches with an id <= patch_id will be returned.
     */
    void set_patch_filter(int patch_id, bool exact);
    /**
     * Disable the existing patch filter
     */
    void reset_patch_filter();
    /**
     * Set the triple pattern to filter by
     * @param triple_pattern The triple pattern that will match all results from this iterator.
     */
    void set_triple_pattern_filter(Triple triple_pattern);
    /**
     * Do not filter on triple patterns.
     */
    void reset_triple_pattern_filter();
    /**
     * Indicate that this iterator should ignore local changes.
     */
    void set_filter_local_changes(bool filter_local_changes);
    /**
     * If the iterator can break early if a non-matching triple was found.
     */
    void set_early_break(bool can_early_break);
    /**
     * If equal additions and deletion elements in the iterators should be combined in a single return element.
     * Default is false.
     */
    void set_squash_equal_addition_deletion(bool squash_equal_addition_deletion);
    /**
     * @return The patch id filter.
     */
    int get_patch_id_filter();
    /**
     * @return If this iterator is iterating over a deletion tree.
     */
    bool is_deletion_tree() const;
    /**
     * @return If this iterator is iterating over an addition tree.
     */
    bool is_addition_tree() const;
    /**
     * Indicate if this iterator should step backwards.
     * @param reverse If it should go reverse.
     */
    void set_reverse(bool reverse);
    /**
     * @return If this iterator goes in reverse.
     */
    bool is_reverse() const;
    /**
     * Point to the next deletion element
     * Can only be called if iterating over a deletion tree.
     * @param key The key the iterator is currently pointing at.
     * @param value The value the iterator is currently pointing at.
     * @param silent_step If the cursor doesn't need to be moved.
     * @return If this next element exists, otherwise the key and value will be invalid and should be ignored.
     */
    bool next_deletion(PatchTreeKey* key, DV* value, bool silent_step = false);
    /**
     * Point to the next addition element
     * Can only be called if iterating over an addition tree.
     * @param key The key the iterator is currently pointing at.
     * @param value The value the iterator is currently pointing at.
     * @return If this next element exists, otherwise the key and value will be invalid and should be ignored.
     */
    bool next_addition(PatchTreeKey* key, PatchTreeAdditionValue* value);
    /**
     * Point to the next element
     * Can only be called if iterating over an addition AND a deletion tree.
     * @param key The key the iterator is currently pointing at.
     * @param value The value the iterator is currently pointing at.
     * @return If this next element exists, otherwise the key and value will be invalid and should be ignored.
     */
    bool next(PatchTreeKey* key, PatchTreeValueBase<DV>* value);
    /**
     * @return The internal deletion cursor, nullable.
     */
    kyotocabinet::DB::Cursor* getDeletionCursor();
    /**
     * @return The internal addition cursor, nullable.
     */
    kyotocabinet::DB::Cursor* getAdditionCursor();
};

typedef PatchTreeIteratorBase<PatchTreeDeletionValue> PatchTreeIterator;
typedef PatchTreeIteratorBase<PatchTreeDeletionValueReduced> PatchTreeIteratorReduced;

#endif //TPFPATCH_STORE_PATCH_TREE_ITERATOR_H
