#include <iostream>

#include <kchashdb.h>

#include "patch_tree_iterator.h"

PatchTreeIterator::PatchTreeIterator(DB::Cursor* cursor_deletions, DB::Cursor* cursor_additions, PatchTreeKeyComparator* comparator)
        : cursor_deletions(cursor_deletions), cursor_additions(cursor_additions), comparator(comparator),
          is_patch_id_filter(false),
          is_patch_id_filter_exact(false), patch_id_filter(-1),
          is_triple_pattern_filter(false), triple_pattern_filter(Triple(0, 0, 0)),
          reverse(false), is_filter_local_changes(false),
          has_temp_key_deletion(false), has_temp_key_addition(false),
          temp_key_deletion(new PatchTreeKey()), temp_key_addition(new PatchTreeKey()) {}

PatchTreeIterator::~PatchTreeIterator() {
    if (cursor_deletions != NULL) delete cursor_deletions;
    if (cursor_additions != NULL) delete cursor_additions;
    delete temp_key_deletion;
    delete temp_key_addition;
}

void PatchTreeIterator::set_patch_filter(int patch_id, bool exact) {
    this->is_patch_id_filter = true;
    this->is_patch_id_filter_exact = exact;
    this->patch_id_filter = patch_id;
}

void PatchTreeIterator::set_triple_pattern_filter(Triple triple_pattern) {
    // Don't do the filtering if we have ? ? ?,
    // this filter then won't have any effect and will only make looping slower.
    if(!Triple::is_all_matching_pattern(triple_pattern)) {
        this->is_triple_pattern_filter = true;
        this->triple_pattern_filter = triple_pattern;
    }
}

void PatchTreeIterator::reset_triple_pattern_filter() {
    this->is_triple_pattern_filter = false;
}

void PatchTreeIterator::set_filter_local_changes(bool filter_local_changes) {
    this->is_filter_local_changes = filter_local_changes;
}

int PatchTreeIterator::get_patch_id_filter() {
    return this->patch_id_filter;
}

bool PatchTreeIterator::is_deletion_tree() const {
    return cursor_deletions != NULL;
}

bool PatchTreeIterator::is_addition_tree() const {
    return cursor_additions != NULL;
}

void PatchTreeIterator::set_reverse(bool reverse) {
    this->reverse = reverse;
}

bool PatchTreeIterator::is_reverse() const {
    return this->reverse;
}

bool PatchTreeIterator::next_deletion(PatchTreeKey* key, PatchTreeDeletionValue* value, bool silent_step) {
    // TODO: abstract code
    if(!is_deletion_tree()) {
        throw std::invalid_argument("Tried to call PatchTreeIterator::next_deletion(PatchTreeKey* key, PatchTreeDeletionValue* value) on non-deletion tree.");
    }
    bool filter_valid = false;
    const char* kbp;
    size_t ksp;
    const char* vbp;
    size_t vsp;

    // Loop over all elements in the tree until we find an element matching the filter (patch_id)
    // If the filter is not enabled, this loop will be executed only once.
    while (!filter_valid) {
        kbp = cursor_deletions->get(&ksp, &vbp, &vsp);
        if (!kbp) return false;
        value->deserialize(vbp, vsp);

        key->deserialize(kbp, ksp);
        delete[] kbp;
        if (is_triple_pattern_filter && !Triple::pattern_match_triple(*key, triple_pattern_filter)) {
            if (can_early_break) {
                // We stop iterating here, because due to the fact that we are always using a triple pattern tree
                // in which our triple patterns will match continuous series of triples, and we will always start
                // iterating on a match, there won't be any matches anymore hereafter.
                return false;
            }
            filter_valid = false;
        } else {
            filter_valid = true;
        }

        if(filter_valid && is_patch_id_filter) {
            if(is_patch_id_filter_exact) {
                long element = value->get_patchvalue_index(patch_id_filter);
                filter_valid = element >= 0;
            } else {
                filter_valid = false;
                for(long i = value->get_size() - 1; i >= 0 && !filter_valid; i--) {
                    PatchTreeDeletionValueElement element = value->get_patch(i);
                    if(element.get_patch_id() <= patch_id_filter) {
                        filter_valid = true;
                    }
                }
            }
        }

        if(filter_valid && is_filter_local_changes) {
            filter_valid = !value->is_local_change(patch_id_filter);
        }

        if(!silent_step || !filter_valid) {
            reverse ? cursor_deletions->step_back() : cursor_deletions->step();
        }
    }
    return true;
}

bool PatchTreeIterator::next_addition(PatchTreeKey* key, PatchTreeAdditionValue* value) {
    if(!is_addition_tree()) {
        throw std::invalid_argument("Tried to call PatchTreeIterator::next_addition(PatchTreeKey* key, PatchTreeAdditionValue* value) on a non-addition tree.");
    }
    bool filter_valid = false;
    const char* kbp;
    size_t ksp;
    const char* vbp;
    size_t vsp;

    // Loop over all elements in the tree until we find an element matching the filter (patch_id)
    // If the filter is not enabled, this loop will be executed only once.
    while (!filter_valid) {
        kbp = cursor_additions->get(&ksp, &vbp, &vsp);
        if (!kbp) {
            return false;
        }
        reverse ? cursor_additions->step_back() : cursor_additions->step();
        value->deserialize(vbp, vsp);

        key->deserialize(kbp, ksp);
        delete[] kbp;
        if (is_triple_pattern_filter && !Triple::pattern_match_triple(*key, triple_pattern_filter)) {
            if (can_early_break) {
                // We stop iterating here, because due to the fact that we are always using a triple pattern tree
                // in which our triple patterns will match continuous series of triples, and we will always start
                // iterating on a match, there won't be any matches anymore hereafter.
                return false;
            } else {
                continue;
            }
        }

        if(is_patch_id_filter) {
            if(is_patch_id_filter_exact) {
                filter_valid = value->is_patch_id(patch_id_filter);
            } else {
                bool done = false;
                for(long i = value->get_size() - 1; i >= 0 && !done; i--) {
                    int patch_id = value->get_patch_id_at(i);
                    if(patch_id >= 0 && patch_id <= patch_id_filter) {
                        done = true;
                        filter_valid = true;
                    }
                }
            }
        } else {
            filter_valid = true;
        }

        if(filter_valid && is_filter_local_changes) {
            filter_valid = !value->is_local_change(patch_id_filter);
        }
    }
    return true;
}

bool PatchTreeIterator::next(PatchTreeKey* key, PatchTreeValue* value) {
    value->set_deletion(false);
    value->set_addition(false);

    // Delegate to +/- iterators if iterating over a pure +/- tree.
    if (is_deletion_tree() && !is_addition_tree()) {
        value->set_deletion(true);
        return next_deletion(key, value->get_deletion());
    }
    if (!is_deletion_tree() && is_addition_tree()) {
        value->set_addition(true);
        return next_addition(key, value->get_addition());
    }

    size_t size;
    const char* data;

    // Temporarily force including local change results, we handle the later.
    bool old_is_filter_local_changes = is_filter_local_changes;
    is_filter_local_changes = false;
    // Call +/- iterators
    // The temp_key_deletion and temp_key_addition are optional previous results,
    // stored in order to avoid having to go over elements more than once.
    bool had_deletion = has_temp_key_deletion || next_deletion(temp_key_deletion, value->get_deletion());
    bool had_addition = has_temp_key_addition || next_addition(temp_key_addition, value->get_addition());
    is_filter_local_changes = old_is_filter_local_changes;

    bool return_addition;
    if (had_deletion != had_addition) {
        return_addition = had_addition;
        has_temp_key_deletion = false;
        has_temp_key_addition = false;
    } else {
        if (!had_deletion && !had_addition) return false;

        // When we get here, both the - and + iterator has a next value, choose the smallest one.
        int comparison = comparator->compare(*temp_key_deletion, *temp_key_addition);
        if (comparison == 0) {
            // Temporarily mark the value as both an + and - to make sure we can perform the following operations
            value->set_deletion(true);
            value->set_addition(true);

            // Return largest patch id
            long deletion_patch_id;
            long addition_patch_id;
            if (is_patch_id_filter) {
                deletion_patch_id = value->get_deletion_patch_id(patch_id_filter, is_patch_id_filter_exact);
                addition_patch_id = value->get_addition_patch_id(patch_id_filter, is_patch_id_filter_exact);
            } else {
                deletion_patch_id = value->get_deletion()->get_patch(
                        value->get_deletion()->get_size() - 1).get_patch_id();
                addition_patch_id = value->get_addition()->get_patch_id_at(value->get_addition()->get_size() - 1);
            }
            return_addition = addition_patch_id > deletion_patch_id;
            has_temp_key_deletion = false;
            has_temp_key_addition = false;
        } else if (comparison > 0) {
            return_addition = true;
            has_temp_key_deletion = true;
            has_temp_key_addition = false;
        } else {
            return_addition = false;
            has_temp_key_deletion = false;
            has_temp_key_addition = true;
        }
    }

    // Correctly mark our return value.
    value->set_deletion(!return_addition);
    value->set_addition(return_addition);

    // If we encountered a local change, and we are filtering those, trigger the next iteration.
    if (is_filter_local_changes && value->is_local_change(patch_id_filter)) {
        return next(key, value);
    }

    // If needed, optimize
    data = (return_addition ? temp_key_addition : temp_key_deletion)->serialize(&size);
    key->deserialize(data, size);
    return true;
}

DB::Cursor *PatchTreeIterator::getDeletionCursor() {
    return this->cursor_deletions;
}

DB::Cursor *PatchTreeIterator::getAdditionCursor() {
    return this->cursor_additions;
}

void PatchTreeIterator::set_early_break(bool can_early_break) {
    this->can_early_break = can_early_break;
}
