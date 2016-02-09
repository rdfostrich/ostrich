#ifndef TPFPATCH_STORE_PATCH_ELEMENTS_H
#define TPFPATCH_STORE_PATCH_ELEMENTS_H

#include "patch.h"

// A triple annotated with addition or deletion
typedef struct PatchElement {
    Triple triple;
    bool addition;
    PatchElement(Triple triple, bool addition) : triple(triple), addition(addition) {}
} PatchElement;

class PatchElements {
protected:
    PatchElement* elements;
    int amount;
public:
    PatchElements();
    ~PatchElements();
    /**
     * Add an element to the patch
     * @param element The element to add
     */
    void add(PatchElement element);
    /**
     * The current size of the patch.
     * @return The size
     */
    int getSize();
    /**
     * Get the patch element at the given position.
     * @param index The index to get a patch element from
     * @return The patch element, will throw an exception if the index is out of bounds.
     */
    PatchElement get(int index);
};

#endif //TPFPATCH_STORE_PATCH_ELEMENTS_H
