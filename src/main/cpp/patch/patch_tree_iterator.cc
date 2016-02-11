#include <iostream>

#include <kchashdb.h>

#include "patch_tree_iterator.h"

PatchTreeIterator::PatchTreeIterator(DB::Cursor *cursor) : cursor(cursor) {}

PatchTreeIterator::~PatchTreeIterator() {
    delete cursor;
}

bool PatchTreeIterator::next(PatchTreeKey** key, PatchTreeValue** value) {
    const char* kbp;
    size_t ksp;
    const char* vbp;
    size_t vsp;
    kbp = cursor->get(&ksp, &vbp, &vsp);
    if(!kbp) return false;
    cursor->step();
    *key = (PatchTreeKey *) kbp;
    *value = (PatchTreeValue *) vbp;
    return true;
}
