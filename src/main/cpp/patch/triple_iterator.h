#ifndef TPFPATCH_STORE_TRIPLE_ITERATOR_H
#define TPFPATCH_STORE_TRIPLE_ITERATOR_H

#include <Iterator.hpp>
#include "patch_tree_iterator.h"

class TripleIterator {
public:
    virtual bool next(Triple* triple) = 0;
};

class PatchTreeTripleIterator : public TripleIterator {
protected:
    PatchTreeIterator* it;
    int patch_id;
    Triple triple_pattern;
public:
    PatchTreeTripleIterator(PatchTreeIterator* it, int patch_id, Triple triple_pattern);
    ~PatchTreeTripleIterator();
    bool next(Triple* triple);
};

class SnapshotTripleIterator : public TripleIterator {
protected:
    IteratorTripleString* snapshot_it;
public:
    SnapshotTripleIterator(IteratorTripleString* snapshot_it);
    ~SnapshotTripleIterator();
    bool next(Triple* triple);
};


#endif //TPFPATCH_STORE_TRIPLE_ITERATOR_H
