#include <Triples.hpp>
#include "triple_iterator.h"

TripleIterator::~TripleIterator() {}

EmptyTripleIterator::EmptyTripleIterator() {}

bool EmptyTripleIterator::next(Triple *triple) {
    return false;
}

#ifdef COMPRESSED_ADD_VALUES
PatchTreeTripleIterator::PatchTreeTripleIterator(PatchTreeIterator* it, Triple triple_pattern, int max_patch_id)
        : it(it), triple_pattern(triple_pattern), max_patch_id(max_patch_id) {}
#else
PatchTreeTripleIterator::PatchTreeTripleIterator(PatchTreeIterator* it, Triple triple_pattern)
        : it(it), triple_pattern(triple_pattern) {}
#endif

PatchTreeTripleIterator::~PatchTreeTripleIterator() {
    delete it;
}

bool PatchTreeTripleIterator::next(Triple *triple) {
    PatchTreeKey key;
#ifdef COMPRESSED_ADD_VALUES
    PatchTreeAdditionValue value(max_patch_id);
#else
    PatchTreeAdditionValue value;
#endif
    bool ret = it->next_addition(&key, &value);
    if(ret) {
        *triple = key;
    }
    return ret;
}

SnapshotTripleIterator::SnapshotTripleIterator(IteratorTripleID* snapshot_it)
        : snapshot_it(snapshot_it) {}

SnapshotTripleIterator::~SnapshotTripleIterator() {
    delete snapshot_it;
}

bool SnapshotTripleIterator::next(Triple* triple) {
    if(snapshot_it->hasNext()) {
        TripleID* triple_id = snapshot_it->next();
        *triple = Triple(triple_id->getSubject(), triple_id->getPredicate(), triple_id->getObject());
        return true;
    }
    return false;
}