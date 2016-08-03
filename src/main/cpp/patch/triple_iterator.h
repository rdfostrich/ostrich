#ifndef TPFPATCH_STORE_TRIPLE_ITERATOR_H
#define TPFPATCH_STORE_TRIPLE_ITERATOR_H

#include <Iterator.hpp>
#include "patch_tree_iterator.h"
#include "triple.h"

class TripleIterator {
public:
    virtual bool next(Triple* triple) = 0;
};

class EmptyTripleIterator : public TripleIterator {
public:
    EmptyTripleIterator();
    bool next(Triple* triple);
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
    IteratorTripleID* snapshot_it;
public:
    SnapshotTripleIterator(IteratorTripleID* snapshot_it);
    ~SnapshotTripleIterator();
    bool next(Triple* triple);
};


#endif //TPFPATCH_STORE_TRIPLE_ITERATOR_H
