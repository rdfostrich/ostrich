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
    void set_filter(int patch_id);
    bool next(PatchTreeKey* key, PatchTreeValue* value);
};


#endif //TPFPATCH_STORE_PATCH_TREE_ITERATOR_H
