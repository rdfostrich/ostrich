#include "patch_tree_value.h"

template <class DV>
PatchTreeValueBase<DV>::PatchTreeValueBase() : addition(new PatchTreeAdditionValue()), deletion(new DV()),
                                   has_addition(false), has_deletion(false) {}

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
    return is_addition(patch_id_start, true) == is_addition(patch_id_end, true)
           && is_deletion(patch_id_start, true) == is_deletion(patch_id_end, true);
}

template class PatchTreeValueBase<PatchTreeDeletionValue>;
template class PatchTreeValueBase<PatchTreeDeletionValueReduced>;
