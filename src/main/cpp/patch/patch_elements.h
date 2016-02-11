#ifndef TPFPATCH_STORE_PATCH_ELEMENTS_H
#define TPFPATCH_STORE_PATCH_ELEMENTS_H

#include <vector>
#include "patch.h"
#include "patch_element.h"

// TODO: rename this to Patch
class PatchElements {
protected:
    std::vector<PatchElement> elements;
public:
    PatchElements();
    /**
     * Add an element to the patch
     * @param element The element to add
     */
    void add(PatchElement element);
    /**
     * The current size of the patch.
     * @return The size
     */
    unsigned long getSize();
    /**
     * Get the patch element at the given position.
     * @param index The index to get a patch element from
     * @return The patch element, will throw an exception if the index is out of bounds.
     */
    PatchElement get(int index);
    /**
     * @return The string representation of this patch.
     */
    string to_string();
};

#endif //TPFPATCH_STORE_PATCH_ELEMENTS_H
