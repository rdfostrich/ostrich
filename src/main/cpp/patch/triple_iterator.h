#ifndef TPFPATCH_STORE_TRIPLE_ITERATOR_H
#define TPFPATCH_STORE_TRIPLE_ITERATOR_H

#include <Iterator.hpp>
#include "patch_tree_iterator.h"
#include "triple.h"

class TripleIterator {
public:
    virtual ~TripleIterator() = 0;
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
    Triple triple_pattern;
#ifdef COMPRESSED_ADD_VALUES
    int max_patch_id;
#endif
public:
#ifdef COMPRESSED_ADD_VALUES
    PatchTreeTripleIterator(PatchTreeIterator* it, Triple triple_pattern, int max_patch_id);
#else
    PatchTreeTripleIterator(PatchTreeIterator* it, Triple triple_pattern);
#endif
    ~PatchTreeTripleIterator();
    bool next(Triple* triple);
};

class SnapshotTripleIterator : public TripleIterator {
protected:
    hdt::IteratorTripleID* snapshot_it;
public:
    SnapshotTripleIterator(hdt::IteratorTripleID* snapshot_it);
    ~SnapshotTripleIterator();
    bool next(Triple* triple);
};



#endif //TPFPATCH_STORE_TRIPLE_ITERATOR_H
