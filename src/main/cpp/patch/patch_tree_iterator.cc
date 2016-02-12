#include <iostream>

#include <kchashdb.h>

#include "patch_tree_iterator.h"

PatchTreeIterator::PatchTreeIterator(DB::Cursor *cursor) : cursor(cursor), filter(false), patch_id_filter(-1) {}

PatchTreeIterator::~PatchTreeIterator() {
    delete cursor;
}

void PatchTreeIterator::set_filter(int patch_id) {
    this->filter = true;
    this->patch_id_filter = patch_id;
}

bool PatchTreeIterator::next(PatchTreeKey* key, PatchTreeValue* value) {
    bool filter_valid = false;
    const char* kbp;
    size_t ksp;
    const char* vbp;
    size_t vsp;

    // Loop over all elements in the tree until we find an element matching the filter (patch_id)
    // If the filter is not enabled, this loop will be executed only once.
    while (!filter_valid) {
        kbp = cursor->get(&ksp, &vbp, &vsp);
        if (!kbp) return false;
        cursor->step();
        value->deserialize(vbp, vsp);
        if(!filter || value->get_patchvalue_index(patch_id_filter) >= 0) {
            filter_valid = true;
            key->deserialize(kbp, ksp); // Small optimization to only deserialize the key when needed.
        }
        delete[] kbp;
        //delete[] vbp; // Apparently kyoto cabinet does not require the values to be deleted
    }
    return true;
}
