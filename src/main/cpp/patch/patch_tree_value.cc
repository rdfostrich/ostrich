#include "patch_tree_value.h"

PatchTreeValue::PatchTreeValue() : addition(new PatchTreeAdditionValue()), deletion(new PatchTreeDeletionValue()),
                                   has_addition(false), has_deletion(false) {}

PatchTreeValue::~PatchTreeValue() {
    delete addition;
    delete deletion;
}

bool PatchTreeValue::is_exact(int patch_id) const {
    return has_addition ? addition->is_patch_id(patch_id) : deletion->get_patchvalue_index(patch_id) >= 0;
}

long PatchTreeValue::get_addition_patch_id(int patch_id, bool exact) const {
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

long PatchTreeValue::get_deletion_patch_id(int patch_id, bool exact) const {
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

bool PatchTreeValue::is_addition(int patch_id, bool exact) const {
    return get_addition_patch_id(patch_id, exact) >= 0;
}

bool PatchTreeValue::is_deletion(int patch_id, bool exact) const {
    return get_deletion_patch_id(patch_id, exact) >= 0;
}

PatchTreeAdditionValue* PatchTreeValue::get_addition() const {
    return addition;
}

PatchTreeDeletionValue* PatchTreeValue::get_deletion() const {
    return deletion;
}

void PatchTreeValue::set_addition(bool has_addition) {
    this->has_addition = has_addition;
}

void PatchTreeValue::set_deletion(bool has_deletion) {
    this->has_deletion = has_deletion;
}

bool PatchTreeValue::is_local_change(int patch_id) const {
    return has_addition ? addition->is_local_change(patch_id) : (has_deletion ? deletion->is_local_change(patch_id) : false);
}

bool PatchTreeValue::is_delta_type_equal(int patch_id_start, int patch_id_end) {
    return is_addition(patch_id_start, false) == is_addition(patch_id_end, false)
           && is_deletion(patch_id_start, false) == is_deletion(patch_id_end, false);
}
