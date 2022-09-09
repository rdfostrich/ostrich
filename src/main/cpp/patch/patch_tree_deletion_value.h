#ifndef TPFPATCH_STORE_PATCH_TREE_DELETION_VALUE_H
#define TPFPATCH_STORE_PATCH_TREE_DELETION_VALUE_H

#include <string>
#include <cstddef>
#include <vector>
#include "triple.h"
#include "interval_list.h"

#define COMPRESSED_DEL_VALUES


typedef long PatchPosition; // TODO: Maybe we can convert this to an int, to reduce tree size

typedef struct PatchPositions {
    // Positions for all triple pattern combinations
    // NOT: S P O (this will always be 0)
    PatchPosition sp_;
    PatchPosition s_o;
    PatchPosition s__;
    PatchPosition _po;
    PatchPosition _p_;
    PatchPosition __o;
    PatchPosition ___;
    PatchPositions() : sp_(-1), s_o(-1), s__(-1), _po(-1), _p_(-1), __o(-1), ___(-1) {}
    PatchPositions(PatchPosition sp_, PatchPosition s_o, PatchPosition s__, PatchPosition _po,
                   PatchPosition _p_, PatchPosition __o, PatchPosition ___)
            : sp_(sp_), s_o(s_o), s__(s__), _po(_po), _p_(_p_), __o(__o), ___(___) {}
    string to_string() const {
        string ret = "{";
        ret += " " + std::to_string(sp_);
        ret += " " + std::to_string(s_o);
        ret += " " + std::to_string(s__);
        ret += " " + std::to_string(_po);
        ret += " " + std::to_string(_p_);
        ret += " " + std::to_string(__o);
        ret += " " + std::to_string(___);
        ret += " }";
        return ret;
    }
    PatchPosition get_by_pattern(const Triple& triple_pattern) const {
        bool s = triple_pattern.get_subject() > 0;
        bool p = triple_pattern.get_predicate() > 0;
        bool o = triple_pattern.get_object() > 0;
        if(s & p & o) return 0;
        if(s & p & !o) return sp_;
        if(s & !p & o) return s_o;
        if(s & !p & !o) return s__;
        if(!s & p & o) return _po;
        if(!s & p & !o) return _p_;
        if(!s & !p & o) return __o;
        /*if(!s & !p & !o)*/ return ___;
    }
    bool operator == (const PatchPositions &rhs) const {
        return this->sp_ == rhs.sp_
               && this->s_o == rhs.s_o
               && this->s__ == rhs.s__
               && this->_po == rhs._po
               && this->_p_ == rhs._p_
               && this->__o == rhs.__o
               && this->___ == rhs.___;
    }
    bool operator != (const PatchPositions &rhs) const {
        return !this->operator==(rhs);
    }
} PatchPositions;

// A PatchTreeDeletionValueElement contains a patch id, a relative patch position and
// an indication whether or not this is an addition or deletion.
class PatchTreeDeletionValueElementBase {
protected:
    int patch_id;
    bool local_change;
public:
    int get_patch_id() const;
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
    bool is_local_change() const;
    /**
     * @return The string representation.
     */
    string to_string() const;
    PatchTreeDeletionValueElementBase() : patch_id(-1), local_change(false) {} // Required for vector#resize
    PatchTreeDeletionValueElementBase(int patch_id) :
            patch_id(patch_id), local_change(false) {}
    PatchTreeDeletionValueElementBase(int patch_id, bool local_change) :
            patch_id(patch_id), local_change(local_change) {}
    bool operator < (const PatchTreeDeletionValueElementBase &rhs) const { return patch_id < rhs.patch_id; }
};

// A PatchTreeDeletionValueElement contains a patch id, a relative patch position and
// an indication whether or not this is an addition or deletion.
class PatchTreeDeletionValueElement : public PatchTreeDeletionValueElementBase {
protected:
    PatchPositions patch_positions;
public:
    const PatchPositions& get_patch_positions() const;
    /**
     * @return The string representation.
     */
    string to_string() const;
    PatchTreeDeletionValueElement() : patch_positions(PatchPositions()) {} // Required for vector#resize
    PatchTreeDeletionValueElement(int patch_id, PatchPositions patch_positions) :
            PatchTreeDeletionValueElementBase(patch_id), patch_positions(patch_positions) {}
    PatchTreeDeletionValueElement(int patch_id) :
            PatchTreeDeletionValueElementBase(patch_id), patch_positions(PatchPositions()) {}
};


#ifndef COMPRESSED_DEL_VALUES
// A PatchTreeDeletionValue in a PatchTree is a sorted list of PatchTreeValueElements
// It contains the information corresponding to one triple in the patch tree.
// It can be seen as a mapping from patch id to the pair of relative patch
// position and element type (addition or deletion).
template <class T>
class PatchTreeDeletionValueBase {
protected:
    std::vector<T> elements;
public:
    PatchTreeDeletionValueBase();
    /**
     * Add the given element.
     * @param element The value element to add
     */
    void add(const T& element);
    /**
     * Get the index of the given patch in this value list.
     * @param patch_id The id of the patch to find
     * @return The index of the given patch in this value list. -1 if not found.
     */
    long get_patchvalue_index(int patch_id) const;
    /**
     * @return The number of PatchTreeDeletionValueElement's stored in this value.
     */
    long get_size() const;
    /**
     * Get the patch of the given element.
     * @param element The element index in this value list. This can be the result of get_patchvalue_index().
     * @return The patch.
     */
    const T& get_patch(long element) const;
    /**
     * @param patch_id The patch id
     * @return The patch.
     */
    const T& get(int patch_id) const;
    /**
     * Check if this element is an element (-) relative to the given patch itself,
     * For example in the series [t1+ t1- t1+], the element at index 1 is a local change,
     * while the others are global changes (with respect to the snapshot).
     * @return If it is a local change.
     */
    bool is_local_change(int patch_id) const;
    /**
     * @return The string representation of this patch.
     */
    string to_string() const;
    /**
     * Serialize this value to a byte array
     * @param size This will contain the size of the returned byte array
     * @return The byte array
     */
    const char* serialize(size_t* size) const;
    /**
     * Deserialize the given byte array to this object.
     * @param data The data to deserialize from.
     * @param size The size of the byte array
     */
    void deserialize(const char* data, size_t size);
    inline PatchTreeDeletionValueBase<PatchTreeDeletionValueElementBase> to_reduced() {
        PatchTreeDeletionValueBase<PatchTreeDeletionValueElementBase> reduced;
        for (long i = 0; i < get_size(); i++) {
            T patch_element = get_patch(i);
            reduced.add(PatchTreeDeletionValueElementBase(patch_element.get_patch_id(), patch_element.is_local_change()));
        }
        return reduced;
    }
};
#else

/*
 * Specialized class to handle DeletionValueElements as intervals
 * Ordering is based on the patch_id.
 * Equality is based on internals (i.e. local_change flag and patch_positions)
 *
 * Templates:
 * T -> DeletionValueElement type
 * I -> Patches ID representation (int or something different in the future)
 * P -> PatchPosition class (for future delta PatchPosition ?)
 */
template<class T, class I, class P>
class DVIntervalList {
private:
    I max;
    IntervalList<I> patches;
    IntervalList<I> local_changes;
    std::map<I, std::pair<I, P>> positions_map;

    bool insert_positions(I patch_id, P positions) {
        bool has_changed = false;
        auto pos = positions_map.lower_bound(patch_id);
        if (pos == positions_map.end()) {
            if (pos != positions_map.begin()) {
                pos--;
                if (pos->first < patch_id && pos->second.first == patch_id) {
                    if (pos->second.second == positions) {
                        positions_map[pos->first] = std::make_pair(max, positions);
                    } else {
                        positions_map[patch_id] = std::make_pair(max, positions);
                    }
                    has_changed = true;
                } else if (pos->second.first < patch_id) {
                    positions_map[patch_id] = std::make_pair(max, positions);
                    has_changed = true;
                }
            } else {
                positions_map.insert(std::make_pair(patch_id, std::make_pair(max, positions)));
                has_changed = true;
            }
        } else {
            if (patch_id < pos->first) {
                pos--;
                if (pos->second.first < patch_id) {
                    pos++;
                    if (pos->second.second == positions) {
                        std::pair<I, P> old_del_val = pos->second;
                        positions_map.erase(pos);
                        positions_map.insert(std::make_pair(patch_id, old_del_val));
                    } else {
                        positions_map[patch_id] = std::make_pair(pos->first, positions);
                    }
                    has_changed = true;
                } else if (pos->second.first == patch_id) {
                    auto tmp_pos = pos;
                    tmp_pos++;
                    if (pos->second.second == positions) {
                        if (tmp_pos->second.second == positions) {
                            if (tmp_pos != positions_map.end()) {
                                std::pair<I, P> old_del_val = tmp_pos->second;
                                positions_map.erase(tmp_pos);
                                pos->second = old_del_val;
                            }
                        } else {
                            I next_change_id = tmp_pos->first;
                            pos->second = std::make_pair(next_change_id, positions);
                        }
                    } else {
                        I next_change_id = tmp_pos->first;
                        positions_map[patch_id] = std::make_pair(next_change_id, positions);
                    }
                    has_changed = true;
                }
            } else if (pos->second.second != positions) {
                pos->second.second = positions; // overwrite
                has_changed = true;
            }
        }
        return has_changed;
    }

    bool delete_positions(I patch_id, P positions) {
        if (positions_map.empty()) {
            return false;
        }
        auto pos = positions_map.lower_bound(patch_id);
        if (pos == positions_map.end() || patch_id < pos->first) {
            pos--;
        }
        if (patch_id < pos->second.first) {
            if (pos->first == patch_id) {
                positions_map.erase(pos);
            } else {
                pos->second.first = patch_id;
            }
            return true;
        }
        return false;
    }

public:
    explicit DVIntervalList() : max(std::numeric_limits<I>::max()),
                                           patches(max),
                                           local_changes(max) {}

    /**
     * Add new element to the list
     * @param element the new element
     */

    bool addition(const T& element, P *positions) {
        I patch = element.get_patch_id();
        bool has_changed = patches.addition(patch);
        if (element.is_local_change()) {
            has_changed |= local_changes.addition(patch);
        }
        if (positions != nullptr)
            has_changed |= insert_positions(element.get_patch_id(), *positions);
        return has_changed;
    }

    bool addition(const PatchTreeDeletionValueElement& element) {
        return addition(element, &element.get_patch_positions());
    }

    bool addition(const PatchTreeDeletionValueElementBase& element) {
        return addition(element, nullptr);
    }

    bool deletion(const T& element, P *positions) {
        I patch = element.get_patch_id();
        bool has_changed = patches.deletion(patch);
        if (element.is_local_change()) {
            has_changed |= local_changes.deletion(patch);
        }
        if (positions != nullptr)
            has_changed |= delete_positions(element.get_patch_id(), *positions);
        return has_changed;
    }

    bool deletion(const PatchTreeDeletionValueElement& element) {
        return deletion(element, &element.get_patch_positions());
    }

    bool deletion(const PatchTreeDeletionValueElementBase& element) {
        return deletion(element, nullptr);
    }

    long get_index(I patch_id) const {
        // TODO
    }

    long size() const {
        // TODO
    }

    const T& get_element_at(long index) const {
        // TODO
    }

};


template <class T>
class PatchTreeDeletionValueBase {
protected:
    DVIntervalList<T, int, PatchPositions> elements;
    int max_patch_id;
public:
    PatchTreeDeletionValueBase(int max_patch_id);
    /**
     * Add the given element.
     * @param element The value element to add
     */
    bool add(const T& element);
    bool del(const T& element);
    /**
     * Get the index of the given patch in this value list.
     * @param patch_id The id of the patch to find
     * @return The index of the given patch in this value list. -1 if not found.
     */
    long get_patchvalue_index(int patch_id) const;
    /**
     * @return The number of PatchTreeDeletionValueElement's stored in this value.
     */
    long get_size() const;
    /**
     * Get the patch of the given element.
     * @param element The element index in this value list. This can be the result of get_patchvalue_index().
     * @return The patch.
     */
    const T& get_patch(long element) const;
    /**
     * @param patch_id The patch id
     * @return The patch.
     */
    const T& get(int patch_id) const;
    /**
     * Check if this element is an element (-) relative to the given patch itself,
     * For example in the series [t1+ t1- t1+], the element at index 1 is a local change,
     * while the others are global changes (with respect to the snapshot).
     * @return If it is a local change.
     */
    bool is_local_change(int patch_id) const;
    /**
     * @return The string representation of this patch.
     */
    string to_string() const;
    /**
     * Serialize this value to a byte array
     * @param size This will contain the size of the returned byte array
     * @return The byte array
     */
    const char* serialize(size_t* size) const;
    /**
     * Deserialize the given byte array to this object.
     * @param data The data to deserialize from.
     * @param size The size of the byte array
     */
    void deserialize(const char* data, size_t size);
    inline PatchTreeDeletionValueBase<PatchTreeDeletionValueElementBase> to_reduced() {
        PatchTreeDeletionValueBase<PatchTreeDeletionValueElementBase> reduced(max_patch_id);
        for (long i = 0; i < get_size(); i++) {
            T patch_element = get_patch(i);
            reduced.add(PatchTreeDeletionValueElementBase(patch_element.get_patch_id(), patch_element.is_local_change()));
        }
        return reduced;
    }
};
#endif

typedef PatchTreeDeletionValueBase<PatchTreeDeletionValueElement> PatchTreeDeletionValue;
typedef PatchTreeDeletionValueBase<PatchTreeDeletionValueElementBase> PatchTreeDeletionValueReduced;

#endif //TPFPATCH_STORE_PATCH_TREE_DELETION_VALUE_H
