#ifndef TPFPATCH_STORE_PATCH_H
#define TPFPATCH_STORE_PATCH_H

#include "triple.h"
#include <vector>
#include <map>
#include <unordered_map>
#include "patch_element.h"
#include "patch_tree_deletion_value.h"
#include "patch_element_comparator.h"
#include "patch_element_iterator.h"

// The amount of triples after which the patch positions should be flushed to disk, to avoid memory issues
#ifndef FLUSH_POSITIONS_COUNT
#define FLUSH_POSITIONS_COUNT 500000
#endif

class PatchIterator { // TODO: rm me? or merge with PatchElementIterator?
public:
    virtual ~PatchIterator(){};
    virtual bool has_next() = 0;
    virtual const PatchElement next() = 0;
};

class PatchIteratorVector : public PatchIterator {
protected:
    std::vector<PatchElement>::const_iterator it;
    std::vector<PatchElement>::const_iterator it_end;
public:
    PatchIteratorVector(std::vector<PatchElement>::const_iterator it, std::vector<PatchElement>::const_iterator it_end);
    ~PatchIteratorVector();
    bool has_next();
    const PatchElement next();
};

class PatchIteratorHashed : public PatchIterator {
protected:
    std::unordered_map<Triple, std::pair<bool, bool>>::const_iterator it;
    std::unordered_map<Triple, std::pair<bool, bool>>::const_iterator it_end;
public:
    PatchIteratorHashed(std::unordered_map<Triple, std::pair<bool, bool>>::const_iterator it, std::unordered_map<Triple, std::pair<bool, bool>>::const_iterator it_end);
    ~PatchIteratorHashed();
    bool has_next();
    const PatchElement next();
};

class Patch {
public:
    /**
     * Add an element to the patch
     * @param element The element to add
     */
    virtual void add(const PatchElement& element) = 0;
    /**
     * Copy all patch elements from the given patch into this patch.
     * @param patch The patch to get all elements from
     */
    void addAll(const Patch& patch);
    /**
     * The current size of the patch.
     * @return The size
     */
    virtual unsigned long get_size() const = 0;
    /**
     * @return The raw string representation of this patch.
     */
    string to_string() const;
    /**
     * @param dict The dictionary to decode from
     * @return The string representation of this patch.
     */
    string to_string(Dictionary& dict) const;
    virtual PatchIterator* iterator() const = 0;
    /**
     * Find the DELETION positions of the given element in this patch based on the pattern-based caches.
     * Additions are thus ignored when doing the counts
     * @param element The element to look for
     * @return The relative positions for all derived triple patterns.
     */
    static PatchPositions positions(const Triple& element,
                                    HashDB& sp_,
                                    HashDB& s_o,
                                    HashDB& s__,
                                    HashDB& _po,
                                    HashDB& _p_,
                                    HashDB& __o,
                                    PatchPosition& ___);
};

class PatchIndexed : public Patch {
public:
    /**
     * Get the patch element at the given position.
     * @param index The index to get a patch element from
     * @return The patch element, will throw an exception if the index is out of bounds.
     */
    virtual const PatchElement& get(long index) const = 0;
    virtual const std::vector<PatchElement>& get_vector() const = 0;
};

// A PatchSorted contains an ordered list of PatchElements
class PatchSorted : public PatchIndexed {
protected:
    std::vector<PatchElement> elements;
    PatchElementComparator* element_comparator;
public:
    PatchSorted(PatchElementComparator* element_comparator);
    PatchSorted(DictionaryManager* dict);
    PatchSorted(PatchElementComparator* element_comparator, std::vector<PatchElement> elements);
    void add(const PatchElement& element);
    /**
     * Add the given patch element in an unsorted manner.
     * @param element The patch element
     */
    void add_unsorted(const PatchElement &element);
    void sort();
    /**
     * Overwrite the element at the given position.
     * @param i The index to overwrite.
     * @param element The element to set.
     */
    void overwrite(long i, const PatchElement& element);
    unsigned long get_size() const;
    const PatchElement& get(long index) const;
    PatchIterator* iterator() const;
    const std::vector<PatchElement>& get_vector() const;
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
     * @param triple The triple to check
     * @return The index of the found triple or -1.
     */
    long index_of_triple(const Triple& triple) const;
    PatchElementIterator* element_iterator();
};

// A PatchUnsorted contains an unordered list of PatchElements
class PatchUnsorted : public PatchIndexed {
protected:
    std::vector<PatchElement> elements;
public:
    void add(const PatchElement& element);
    unsigned long get_size() const;
    const PatchElement& get(long index) const;
    PatchIterator* iterator() const;
    const std::vector<PatchElement>& get_vector() const;
};

// A PatchHashed contains an unordered set of PatchElements
class PatchHashed : public Patch {
protected:
    std::unordered_map<Triple, std::pair<bool, bool>> elements;
public:
    void add(const PatchElement& element);
    unsigned long get_size() const;
    PatchIterator* iterator() const;
    /**
     * Copy all patch elements from this and the given patch into a new patch patch.
     * This is optimized for merging large patches.
     * @param patch The patch to get all elements from
     */
    PatchSorted* join_sorted(const PatchIndexed &patch, PatchElementComparator *element_comparator);
};

#endif //TPFPATCH_STORE_PATCH_H
