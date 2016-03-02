#ifndef TPFPATCH_STORE_TRIPLE_ITERATOR_H
#define TPFPATCH_STORE_TRIPLE_ITERATOR_H


#include "patch_tree_iterator.h"

class TripleIterator {
protected:
    PatchTreeIterator* it;
    int patch_id;
    Triple triple_pattern;
public:
    TripleIterator(PatchTreeIterator* it, int patch_id, Triple triple_pattern);
    ~TripleIterator();
    bool next(Triple* positioned_triple);
};


#endif //TPFPATCH_STORE_TRIPLE_ITERATOR_H
