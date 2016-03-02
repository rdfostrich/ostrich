#include "triple_iterator.h"

TripleIterator::TripleIterator(PatchTreeIterator* it, int patch_id, Triple triple_pattern)
        : it(it), patch_id(patch_id), triple_pattern(triple_pattern) {}

TripleIterator::~TripleIterator() {
    delete it;
}

bool TripleIterator::next(Triple *triple) {
    PatchTreeKey key;
    PatchTreeAdditionValue value;
    bool ret = it->next_addition(&key, &value);
    if(ret) {
        *triple = key;
    }
    return ret;
}