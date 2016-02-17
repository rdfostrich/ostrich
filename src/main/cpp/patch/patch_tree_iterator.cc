#include <iostream>

#include <kchashdb.h>

#include "patch_tree_iterator.h"

PatchTreeIterator::PatchTreeIterator(DB::Cursor *cursor)
        : cursor(cursor), is_patch_id_filter(false), is_patch_id_filter_exact(false), patch_id_filter(-1),
          is_addition_filter(false), addition_filter(-1) {}

PatchTreeIterator::~PatchTreeIterator() {
    delete cursor;
}

void PatchTreeIterator::set_patch_filter(int patch_id, bool exact) {
    this->is_patch_id_filter = true;
    this->is_patch_id_filter_exact = exact;
    this->patch_id_filter = patch_id;
}

void PatchTreeIterator::set_type_filter(bool addition) {
    this->is_addition_filter = true;
    this->addition_filter = addition;
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

        if(is_patch_id_filter) {
            if(is_patch_id_filter_exact) {
                long element = value->get_patchvalue_index(patch_id_filter);
                if(element >= 0
                   && (!is_addition_filter || value->get_patch(element).is_addition() == addition_filter)) {
                    filter_valid = true;
                }
            } else {
                for(long i = value->get_size() - 1; i >= 0 && !filter_valid; i--) {
                    PatchTreeValueElement element = value->get_patch(i);
                    if(element.get_patch_id() <= patch_id_filter) {
                        filter_valid = (!is_addition_filter || element.is_addition() == addition_filter);
                    }
                }
            }
        } else {
            if(is_addition_filter) {
                filter_valid = value->get_patch(value->get_size() - 1).is_addition() == addition_filter;
            } else {
                filter_valid = true;
            }
        }

        if(filter_valid) {
            key->deserialize(kbp, ksp); // Small optimization to only deserialize the key when needed.
        }

        delete[] kbp;
        //delete[] vbp; // Apparently kyoto cabinet does not require the values to be deleted
    }
    return true;
}
