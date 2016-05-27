#ifndef TPFPATCH_STORE_PATCH_H
#define TPFPATCH_STORE_PATCH_H

#include <vector>
#include <map>
#include <unordered_map>
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
    void add(const PatchElement& element);
    /**
     * Overwrite the element at the given position.
     * @param i The index to overwrite.
     * @param element The element to set.
     */
    void overwrite(long i, const PatchElement& element);
    /**
     * Copy all patch elements from the given patch into this patch.
     * @param patch The patch to get all elements from
     */
    void addAll(const Patch& patch);
    /**
     * The current size of the patch.
     * @return The size
     */
    unsigned long get_size() const;
    /**
     * Get the patch element at the given position.
     * @param index The index to get a patch element from
     * @return The patch element, will throw an exception if the index is out of bounds.
     */
    const PatchElement& get(long index) const;
    /**
     * Find the DELETION positions of the given element in this patch based on the pattern-based caches.
     * Additions are thus ignored when doing the counts
     * @param element The element to look for
     * @return The relative positions for all derived triple patterns.
     */
    PatchPositions positions(const PatchElement& element,
                              unordered_map<string, PatchPosition>& sp_,
                              unordered_map<string, PatchPosition>& s_o,
                              unordered_map<string, PatchPosition>& s__,
                              unordered_map<string, PatchPosition>& _po,
                              unordered_map<string, PatchPosition>& _p_,
                              unordered_map<string, PatchPosition>& __o,
                              PatchPosition& ___) const;
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
    PatchPosition position_of_pattern(const PatchElement& element, bool s, bool p, bool o, bool type) const;
    /**
     * Find the position of the given element in this patch.
     * Note that the element does not *have to* be present in the patch, it will just return the position
     * at which it would be present.
     * @param element The element to look for
     * @return The relative position
     */
    PatchPosition position_of(const PatchElement& element) const;
    /**
     * Find the position of the given element in this patch.
     * @param element The element to look for
     * @return The relative position, -1 if not present in this patch.
     */
    PatchPosition position_of_strict(const PatchElement& element) const;
    /**
     * @return The string representation of this patch.
     */
    string to_string() const;
    /**
     * @param triple The triple to check
     * @return The index of the found triple or -1.
     */
    long index_of_triple(const Triple& triple) const;
    /**
     * Derive a patch that detects local changes and correctly
     * removed those patch elements that have been removed due to the
     * local changes.
     * @return The derived patch, with local change markings.
     */
    Patch apply_local_changes() const;
};

#endif //TPFPATCH_STORE_PATCH_H
