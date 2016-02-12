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
    bool filter;
    int patch_id_filter;
public:
    PatchTreeIterator(DB::Cursor* cursor);
    ~PatchTreeIterator();
    /**
     * Set the patch id to filter by
     * @param patch_id The patch id to filter by
     */
    void set_filter(int patch_id);
    /**
     * Point to the next element
     * @param key The key the iterator is currently pointing at.
     * @param value The value the iterator is currently pointing at.
     * @return If this next element exists, otherwise the key and value will be invalid and should be ignored.
     */
    bool next(PatchTreeKey* key, PatchTreeValue* value);
};


#endif //TPFPATCH_STORE_PATCH_TREE_ITERATOR_H
