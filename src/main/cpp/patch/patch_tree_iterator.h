#ifndef TPFPATCH_STORE_PATCH_TREE_ITERATOR_H
#define TPFPATCH_STORE_PATCH_TREE_ITERATOR_H

#include <string>
#include <kchashdb.h>
#include "patch.h"

using namespace std;
using namespace kyotocabinet;

class PatchTreeIterator {
private:
    DB::Cursor* cursor;
public:
    PatchTreeIterator(DB::Cursor* cursor);
    ~PatchTreeIterator();
    bool next(PatchTreeKey** key, bool** value); // TODO: modify bool
};


#endif //TPFPATCH_STORE_PATCH_TREE_ITERATOR_H
