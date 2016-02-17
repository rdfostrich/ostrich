#ifndef TPFPATCH_STORE_PATCH_TREE_ITERATOR_H
#define TPFPATCH_STORE_PATCH_TREE_ITERATOR_H

#include <string>
#include <kchashdb.h>
#include "patch.h"
#include "patch_tree_value.h"

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
     * Indicate that this iterator should step backwards.
     */
    void set_reverse();
    /**
     * Point to the next element
     * @param key The key the iterator is currently pointing at.
     * @param value The value the iterator is currently pointing at.
     * @return If this next element exists, otherwise the key and value will be invalid and should be ignored.
     */
    bool next(PatchTreeKey* key, PatchTreeValue* value);
};


#endif //TPFPATCH_STORE_PATCH_TREE_ITERATOR_H
