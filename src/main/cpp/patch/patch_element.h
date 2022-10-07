#ifndef TPFPATCH_STORE_PATCH_ELEMENT_H
#define TPFPATCH_STORE_PATCH_ELEMENT_H

#include "triple.h"
#include "patch_tree_key_comparator.h"

// A PatchElement is a Triple annotated with addition or deletion
class PatchElement {
protected:
    Triple triple;
    bool addition;
    bool local_change;
public:
    PatchElement();
    PatchElement(const Triple& triple, bool addition);
    PatchElement(const Triple& triple, bool addition, bool local_change);
    /**
     * Set the triple
     * @param triple The new triple
     */
    void set_triple(const Triple triple);
    /**
     * Get the triple
     * @return The triple
     */
    const Triple& get_triple() const;
    /**
     * Set the addition flag
     * @param addition If this is an addition.
     */
    void set_addition(bool addition);
    /**
     * Check if this element is an addition, otherwise it is a deletion
     * @return If it is an addition
     */
    bool is_addition() const;
    /**
     * Set the local change flag
     * @param local_change If this is a local change.
     */
    void set_local_change(bool local_change);
    /**
     * Check if this element is an element (+/-) relative to this patch itself,
     * For example in the series [t1+ t1- t1+], the element at index 1 is a local change,
     * while the others are global changes (with respect to the snapshot).
     * @return If it is a local change.
     */
    bool is_local_change() const;
    /**
     * @return The raw string representation of this patch.
     */
    const string to_string() const;
    /**
     * @param dict The dictionary to decode from
     * @return The string representation of this patch.
     */
    const string to_string(hdt::Dictionary& dict) const;
    //bool operator < (const PatchElement &rhs) const { return triple < rhs.triple || (triple == rhs.triple && !addition && rhs.addition); }
    bool operator == (const PatchElement &rhs) const { return triple == rhs.triple && addition == rhs.addition; }
    bool operator != (const PatchElement &rhs) const { return !operator==(rhs); }
};

#endif //TPFPATCH_STORE_PATCH_ELEMENT_H
