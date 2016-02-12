#ifndef TPFPATCH_STORE_PATCH_ELEMENT_H
#define TPFPATCH_STORE_PATCH_ELEMENT_H

#include "triple.h"

// A PatchElement is a Triple annotated with addition or deletion
class PatchElement {
protected:
    Triple triple;
    bool addition;
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
     * @return The string representation of this patch.
     */
    string to_string();
    bool operator < (const PatchElement &rhs) const { return triple < rhs.triple || (triple == rhs.triple && !addition && rhs.addition); }
    bool operator == (const PatchElement &rhs) const { return triple == rhs.triple && addition == rhs.addition; }
};


#endif //TPFPATCH_STORE_PATCH_ELEMENT_H
