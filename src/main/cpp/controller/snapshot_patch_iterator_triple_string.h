#ifndef TPFPATCH_STORE_SNAPSHOT_ITERATOR_TRIPLE_STRING_H
#define TPFPATCH_STORE_SNAPSHOT_ITERATOR_TRIPLE_STRING_H

#include <Triples.hpp>
#include <Iterator.hpp>
#include "../patch/positioned_triple_iterator.h"
#include "../patch/patch_tree.h"

class SnapshotPatchIteratorTripleString : public TripleIterator {
private:
    IteratorTripleString* snapshot_it;
    PositionedTripleIterator* deletion_it;
    PatchTreeTripleIterator* addition_it;
public:
    SnapshotPatchIteratorTripleString(IteratorTripleString* snapshot_it, PositionedTripleIterator* deletion_it, PatchTreeTripleIterator * addition_it);
    bool next(Triple* triple);
};


#endif //TPFPATCH_STORE_SNAPSHOT_ITERATOR_TRIPLE_STRING_H
