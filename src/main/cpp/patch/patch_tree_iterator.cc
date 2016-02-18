#include <iostream>

#include <kchashdb.h>

#include "patch_tree_iterator.h"

PatchTreeIterator::PatchTreeIterator(DB::Cursor *cursor)
        : cursor(cursor), is_patch_id_filter(false), is_patch_id_filter_exact(false), patch_id_filter(-1),
          is_addition_filter(false), addition_filter(-1),
          is_triple_pattern_filter(false), triple_pattern_filter(Triple("", "", "")),
          reverse(false) {}

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

void PatchTreeIterator::set_triple_pattern_filter(Triple triple_pattern) {
    // Don't do the filtering if we have ? ? ?,
    // this filter then won't have any effect and will only make looping slower.
    if(!Triple::is_all_matching_pattern(triple_pattern)) {
        this->is_triple_pattern_filter = true;
        this->triple_pattern_filter = triple_pattern;
    }
}

void PatchTreeIterator::set_reverse() {
    this->reverse = true;
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
        reverse ? cursor->step_back() : cursor->step();
        value->deserialize(vbp, vsp);

        if(is_patch_id_filter) {
            if(is_patch_id_filter_exact) {
                long element = value->get_patchvalue_index(patch_id_filter);
                filter_valid = element >= 0
                               && (!is_addition_filter || (value->get_patch(element).is_addition() == addition_filter));
            } else {
                bool done = false;
                for(long i = value->get_size() - 1; i >= 0 && !done; i--) {
                    PatchTreeValueElement element = value->get_patch(i);
                    if(element.get_patch_id() <= patch_id_filter) {
                        done = true;
                        filter_valid = (!is_addition_filter || (element.is_addition() == addition_filter));
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
            filter_valid = !is_triple_pattern_filter || Triple::pattern_match_triple(*key, triple_pattern_filter);
        }

        delete[] kbp;
        //delete[] vbp; // Apparently kyoto cabinet does not require the values to be deleted
    }
    return true;
}
