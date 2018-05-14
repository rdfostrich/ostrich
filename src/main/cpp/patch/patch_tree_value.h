#ifndef TPFPATCH_STORE_PATCH_TREE_VALUE_H
#define TPFPATCH_STORE_PATCH_TREE_VALUE_H


#include "patch_tree_addition_value.h"
#include "patch_tree_deletion_value.h"

template <class DV>
class PatchTreeValueBase {
protected:
    PatchTreeAdditionValue* addition;
    DV* deletion;
    bool has_addition;
    bool has_deletion;
public:
    PatchTreeValueBase();
    ~PatchTreeValueBase();

    /**
     * if this value is exactly defined for the given patch id.
     * @param The patch id to match.
     */
    bool is_exact(int patch_id) const;
    /**
     * Get the largest addition patch id smaller or equal to the given patch id.
     * @param The patch id to match.
     * @param If the patch id must match exactly, otherwise previous patch id's will also be matched
     */
    long get_addition_patch_id(int patch_id, bool exact) const;
    /**
     * Get the largest deletion patch id smaller or equal to the given patch id.
     * @param The patch id to match.
     * @param If the patch id must match exactly, otherwise previous patch id's will also be matched
     */
    long get_deletion_patch_id(int patch_id, bool exact) const;
    /**
     * If this value is an addition.
     * @param The patch id to match.
     * @param If the patch id must match exactly, otherwise previous patch id's will also be matched
     */
    bool is_addition(int patch_id, bool exact) const;
    /**
     * If this value is a deletion.
     * @param The patch id to match.
     * @param If the patch id must match exactly, otherwise previous patch id's will also be matched
     */
    bool is_deletion(int patch_id, bool exact) const;
    /**
     * This check only uses metadata from the addition and deletion values.
     * @return Check if this element exists in the snapshot.
     */
    bool exists_in_snapshot() const;
    /**
     * If this value is present in the given patch id.
     * @param The patch id to match.
     */
    bool is_present(int patch_id) const;
    PatchTreeAdditionValue* get_addition() const;
    DV* get_deletion() const;
    void set_addition(bool has_addition);
    void set_deletion(bool has_deletion);
    /**
     * Check if this element is an element (+/-) relative to the given patch itself,
     * For example in the series [t1+ t1- t1+], the element at index 1 is a local change,
     * while the others are global changes (with respect to the snapshot).
     * @return If it is a local change.
     */
    bool is_local_change(int patch_id) const;
    /**
     * Check if the addition or deletion tag is the same between versions.
     * @param patch_id_start The first patch id
     * @param patch_id_end The second patch id
     * @return If addition/deletion is the same.
     */
    bool is_delta_type_equal(int patch_id_start, int patch_id_end);
};

typedef PatchTreeValueBase<PatchTreeDeletionValue> PatchTreeValue;
typedef PatchTreeValueBase<PatchTreeDeletionValueReduced> PatchTreeValueReduced;

#endif //TPFPATCH_STORE_PATCH_TREE_VALUE_H
