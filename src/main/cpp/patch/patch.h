#ifndef TPFPATCH_STORE_PATCH_ELEMENTS_H
#define TPFPATCH_STORE_PATCH_ELEMENTS_H

#include <vector>
#include "patch.h"
#include "patch_element.h"

// A key in the PatchTree is a triple
typedef Triple PatchTreeKey;

class Patch {
protected:
    std::vector<PatchElement> elements;
public:
    Patch();
    /**
     * Add an element to the patch
     * @param element The element to add
     */
    void add(PatchElement element);
    /**
     * Copy all patch elements from the given patch into this patch.
     * @param patch The patch to get all elements from
     */
    void addAll(Patch patch);
    /**
     * The current size of the patch.
     * @return The size
     */
    unsigned long get_size();
    /**
     * Get the patch element at the given position.
     * @param index The index to get a patch element from
     * @return The patch element, will throw an exception if the index is out of bounds.
     */
    PatchElement get(int index);
    /**
     * Find the position of the given element in this patch.
     * Note that the element does not *have to* be present in the patch, it will just return the position
     * at which it would be present.
     * @param The element to look for
     * @return The relative position
     */
    int position_of(PatchElement element);
    /**
     * Find the position of the given element in this patch.
     * @param The element to look for
     * @return The relative position, -1 if not present in this patch.
     */
    int position_of_strict(PatchElement element);
    /**
     * @return The string representation of this patch.
     */
    string to_string();
};

#endif //TPFPATCH_STORE_PATCH_ELEMENTS_H
