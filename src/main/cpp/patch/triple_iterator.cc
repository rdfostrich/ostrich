#include <Triples.hpp>
#include "triple_iterator.h"

PatchTreeTripleIterator::PatchTreeTripleIterator(PatchTreeIterator* it, int patch_id, Triple triple_pattern)
        : it(it), patch_id(patch_id), triple_pattern(triple_pattern) {}

PatchTreeTripleIterator::~PatchTreeTripleIterator() {
    delete it;
}

bool PatchTreeTripleIterator::next(Triple *triple) {
    PatchTreeKey key;
    PatchTreeAdditionValue value;
    bool ret = it->next_addition(&key, &value);
    if(ret) {
        *triple = key;
    }
    return ret;
}

SnapshotTripleIterator::SnapshotTripleIterator(IteratorTripleString* snapshot_it)
        : snapshot_it(snapshot_it) {}

SnapshotTripleIterator::~SnapshotTripleIterator() {
    delete snapshot_it;
}

bool SnapshotTripleIterator::next(Triple* triple) {
    if(snapshot_it->hasNext()) {
        TripleString* triple_string = snapshot_it->next();
        *triple = Triple(triple_string->getSubject(), triple_string->getPredicate(), triple_string->getObject());;
        return true;
    }
    return false;
}