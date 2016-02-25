#ifndef TPFPATCH_STORE_PATCH_ELEMENT_H
#define TPFPATCH_STORE_PATCH_ELEMENT_H

#include "triple.h"

// A PatchElement is a Triple annotated with addition or deletion
class PatchElement {
protected:
    Triple triple;
    bool addition;
    bool local_change;
public:
    PatchElement(Triple triple, bool addition);
    /**
     * Get the triple
     * @return The triple
     */
    Triple get_triple();
    /**
     * Check if this element is an addition, otherwise it is a deletion
     * @return If it is an addition
     */
    bool is_addition();
    /**
     * Mark this patch element as being a local change.
     */
    void set_local_change();
    /**
     * Check if this element is an element (+/-) relative to this patch itself,
     * For example in the series [t1+ t1- t1+], the element at index 1 is a local change,
     * while the others are global changes (with respect to the snapshot).
     * @return If it is a local change.
     */
    bool is_local_change();
    /**
     * @return The string representation of this patch.
     */
    string to_string();
    bool operator < (const PatchElement &rhs) const { return triple < rhs.triple || (triple == rhs.triple && !addition && rhs.addition); }
    bool operator == (const PatchElement &rhs) const { return triple == rhs.triple && addition == rhs.addition; }
};


#endif //TPFPATCH_STORE_PATCH_ELEMENT_H
