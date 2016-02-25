#ifndef TPFPATCH_STORE_PATCH_ELEMENTS_H
#define TPFPATCH_STORE_PATCH_ELEMENTS_H

#include <vector>
#include "patch.h"
#include "patch_element.h"
#include "patch_tree_value.h"

// A key in the PatchTree is a triple
typedef Triple PatchTreeKey;

// A Patch contains an ordered list of PatchElements
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
    PatchElement get(long index);
    /**
     * Find the DELETION positions of the given element in this patch for all triple patterns.
     * Additions are thus ignored when doing the counts
     * @param element The element to look for
     * @return The relative positions for all derived triple patterns.
     */
    PatchPositions positions(PatchElement element);
    /**
     * Find the position of the given element in this patch.
     * The boolean parameters are used to virtually include or exclude certain elements.
     * For example, if `s` is true, then only the elements from this patch that have the same subject as the
     * given element will be taken into account when calculating its position.
     * @param element The element to look for
     * @param s If the subject must be matched
     * @param p If the predicate must be matched
     * @param o If the object must be matched
     * @param type If the type (addition/deletion) must be matched
     * @return The relative position, -1 if not present in this patch.
     * @deprecated Use positions() instead to calculate all positions at once
     */
    PatchPosition position_of_pattern(PatchElement element, bool s, bool p, bool o, bool type);
    /**
     * Find the position of the given element in this patch.
     * Note that the element does not *have to* be present in the patch, it will just return the position
     * at which it would be present.
     * @param element The element to look for
     * @return The relative position
     */
    PatchPosition position_of(PatchElement element);
    /**
     * Find the position of the given element in this patch.
     * @param element The element to look for
     * @return The relative position, -1 if not present in this patch.
     */
    PatchPosition position_of_strict(PatchElement element);
    /**
     * @return The string representation of this patch.
     */
    string to_string();
    /**
     * @param triple The triple to check
     * @return The index of the found triple or -1.
     */
    long index_of_triple(Triple triple);
};

#endif //TPFPATCH_STORE_PATCH_ELEMENTS_H
