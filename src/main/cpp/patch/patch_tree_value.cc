#include "patch_tree_value.h"

#ifdef COMPRESSED_ADD_VALUES
template <class DV>
PatchTreeValueBase<DV>::PatchTreeValueBase(int max_patch_id) : addition(new PatchTreeAdditionValue(max_patch_id)), deletion(new DV()),
                                               has_addition(false), has_deletion(false) {}
#else
template <class DV>
PatchTreeValueBase<DV>::PatchTreeValueBase() : addition(new PatchTreeAdditionValue()), deletion(new DV()),
                                               has_addition(false), has_deletion(false) {}
#endif

template <class DV>
PatchTreeValueBase<DV>::~PatchTreeValueBase() {
    delete addition;
    delete deletion;
}

template <class DV>
bool PatchTreeValueBase<DV>::is_exact(int patch_id) const {
    return has_addition ? addition->is_patch_id(patch_id) : deletion->get_patchvalue_index(patch_id) >= 0;
}

template <class DV>
long PatchTreeValueBase<DV>::get_addition_patch_id(int patch_id, bool exact) const {
    if (has_addition) {
        if (exact) {
            return addition->get_patchvalue_index(patch_id);
        } else {
            for(long i = addition->get_size() - 1; i >= 0; i--) {
                long patch_id_at = addition->get_patch_id_at(i);
                if (patch_id_at <= patch_id) {
                    return patch_id_at;
                }
            }
        }
    }
    return -1;
}

template <class DV>
long PatchTreeValueBase<DV>::get_deletion_patch_id(int patch_id, bool exact) const {
    if (has_deletion) {
        if (exact) {
            return deletion->get_patchvalue_index(patch_id);
        } else {
            for(long i = deletion->get_size() - 1; i >= 0; i--) {
                long patch_id_at = deletion->get_patch(i).get_patch_id();
                if (patch_id_at <= patch_id) {
                    return patch_id_at;
                }
            }
        }
    }
    return -1;
}

template <class DV>
bool PatchTreeValueBase<DV>::is_addition(int patch_id, bool exact) const {
    return get_addition_patch_id(patch_id, exact) >= 0;
}

template <class DV>
bool PatchTreeValueBase<DV>::is_deletion(int patch_id, bool exact) const {
    return get_deletion_patch_id(patch_id, exact) >= 0;
}

template <class DV>
bool PatchTreeValueBase<DV>::exists_in_snapshot() const {
    // We can only know if this element exists in the snapshot
    // if the first deletion comes before the first addition (or there is no later addition).
    if (has_deletion) {
        if (has_addition) {
            return deletion->get_patch(0).get_patch_id() < addition->get_patch_id_at(0);
        }
        return true;
    }
    return false;
}

template <class DV>
bool PatchTreeValueBase<DV>::is_present(int patch_id) const {
    long addition = get_addition_patch_id(patch_id, true);
    long deletion = get_deletion_patch_id(patch_id, true);
    if (addition == -1 && deletion == -1) {
        // In this case, the snapshot applies.
        return exists_in_snapshot();
    }
    return deletion < addition; // otherwise addition > deletion (addition == deletion is not possible)
}

template <class DV>
PatchTreeAdditionValue* PatchTreeValueBase<DV>::get_addition() const {
    return addition;
}

template <class DV>
DV* PatchTreeValueBase<DV>::get_deletion() const {
    return deletion;
}

template <class DV>
void PatchTreeValueBase<DV>::set_addition(bool has_addition) {
    this->has_addition = has_addition;
}

template <class DV>
void PatchTreeValueBase<DV>::set_deletion(bool has_deletion) {
    this->has_deletion = has_deletion;
}

template <class DV>
bool PatchTreeValueBase<DV>::is_local_change(int patch_id) const {
    return has_addition ? addition->is_local_change(patch_id) : (has_deletion ? deletion->is_local_change(patch_id) : false);
}

template <class DV>
bool PatchTreeValueBase<DV>::is_delta_type_equal(int patch_id_start, int patch_id_end) {
    return is_present(patch_id_start) == is_present(patch_id_end);
}

template class PatchTreeValueBase<PatchTreeDeletionValue>;
template class PatchTreeValueBase<PatchTreeDeletionValueReduced>;
