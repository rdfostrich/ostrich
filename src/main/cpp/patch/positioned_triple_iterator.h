#ifndef TPFPATCH_STORE_POSITIONED_TRIPLE_ITERATOR_H
#define TPFPATCH_STORE_POSITIONED_TRIPLE_ITERATOR_H


#include "patch_tree_iterator.h"

typedef struct PositionedTriple {
    Triple triple;
    long position;
} PositionedTriple;

class PositionedTripleIterator {
protected:
    PatchTreeIterator* it;
    int patch_id;
    Triple triple_pattern;
public:
    PositionedTripleIterator(PatchTreeIterator* it, int patch_id, Triple triple_pattern);
    ~PositionedTripleIterator();
    bool next(PositionedTriple* positioned_triple, bool silent_step = false, bool get_position = true);
    PatchTreeIterator* getPatchTreeIterator();
};


#endif //TPFPATCH_STORE_POSITIONED_TRIPLE_ITERATOR_H
