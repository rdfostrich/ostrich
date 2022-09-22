#ifndef TPFPATCH_STORE_PATCH_TREE_DELETION_VALUE_H
#define TPFPATCH_STORE_PATCH_TREE_DELETION_VALUE_H

#include <string>
#include <cstddef>
#include <vector>
#include <memory>
#include "triple.h"
#include "interval_list.h"

#define COMPRESSED_DEL_VALUES

typedef long PatchPosition;

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

//    PatchPositions(const PatchPositions &other) = default;
//    PatchPositions& operator=(const PatchPositions& other) = default;

    std::string to_string() const {
        std::string ret = "{";
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

    PatchPosition get_by_pattern(const Triple &triple_pattern) const {
        bool s = triple_pattern.get_subject() > 0;
        bool p = triple_pattern.get_predicate() > 0;
        bool o = triple_pattern.get_object() > 0;
        if (s & p & o) return 0;
        if (s & p & !o) return sp_;
        if (s & !p & o) return s_o;
        if (s & !p & !o) return s__;
        if (!s & p & o) return _po;
        if (!s & p & !o) return _p_;
        if (!s & !p & o) return __o;
        /*if(!s & !p & !o)*/ return ___;
    }

    bool operator==(const PatchPositions &rhs) const {
        return this->sp_ == rhs.sp_
               && this->s_o == rhs.s_o
               && this->s__ == rhs.s__
               && this->_po == rhs._po
               && this->_p_ == rhs._p_
               && this->__o == rhs.__o
               && this->___ == rhs.___;
    }

    bool operator!=(const PatchPositions &rhs) const {
        return !this->operator==(rhs);
    }
} PatchPositions;

// A PatchTreeDeletionValueElement contains a patch id, a relative patch position and
// an indication whether this is an addition or deletion.
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

    explicit PatchTreeDeletionValueElementBase(int patch_id) :
            patch_id(patch_id), local_change(false) {}

    PatchTreeDeletionValueElementBase(int patch_id, bool local_change) :
            patch_id(patch_id), local_change(local_change) {}

    // Ugly, but required to make the interval deletion_value templated code work
    PatchTreeDeletionValueElementBase(int patch_id, bool local_change, const PatchPositions& p) :
            patch_id(patch_id), local_change(local_change) {}

    bool operator<(const PatchTreeDeletionValueElementBase &rhs) const { return patch_id < rhs.patch_id; }

    bool operator==(const PatchTreeDeletionValueElementBase &rhs) const {
        return patch_id == rhs.patch_id && local_change == rhs.local_change;
    }

    static bool has_positions() {
        return false;
    }

    /**
     * Dummy function to make the new interval deletion value code compile
     * @return nothing important
     */
    static PatchPositions get_patch_positions() {
        return {};
    }

};

// A PatchTreeDeletionValueElement contains a patch id, a relative patch position and
// an indication whether this is an addition or deletion.
class PatchTreeDeletionValueElement : public PatchTreeDeletionValueElementBase {
protected:
    PatchPositions patch_positions;
public:
    const PatchPositions &get_patch_positions() const;

    /**
     * @return The string representation.
     */
    std::string to_string() const;

    PatchTreeDeletionValueElement() : patch_positions(PatchPositions()) {} // Required for vector#resize

    PatchTreeDeletionValueElement(int patch_id, const PatchPositions& patch_positions) :
            PatchTreeDeletionValueElementBase(patch_id), patch_positions(patch_positions) {}

    explicit PatchTreeDeletionValueElement(int patch_id) :
            PatchTreeDeletionValueElementBase(patch_id), patch_positions(PatchPositions()) {}

    PatchTreeDeletionValueElement(int patch_id, bool local_change) :
            PatchTreeDeletionValueElementBase(patch_id, local_change), patch_positions(PatchPositions()) {}

    PatchTreeDeletionValueElement(int patch_id, bool local_change, const PatchPositions& patch_positions) :
            PatchTreeDeletionValueElementBase(patch_id, local_change), patch_positions(patch_positions) {}

    bool operator==(const PatchTreeDeletionValueElement &rhs) const {
        return patch_id == rhs.patch_id && local_change == rhs.local_change && patch_positions == rhs.patch_positions;
    }

    static bool has_positions() {
        return true;
    }
};

/*
 * Specialized class to handle DeletionValueElements as intervals
 * Ordering is based on the patch_id.
 * Equality is based on internals (i.e. local_change flag and patch_positions)
 *
 * Templates:
 * T -> DeletionValueElement type
 * I -> Patches ID representation (int or something different in the future)
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
                } else if (pos->second.second != positions) {
                    pos->second.first = patch_id;
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

    bool delete_positions(I patch_id) {
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

    /**
     * Get the PatchPositions for the given patch id
     * @param patch_id the patch_id
     * @param outer_limit the maximum value stored internally, i.e. the max patch_id
     * @return PatchPositions if found or nullptr if not found
     */
    std::unique_ptr<P> get_position(I patch_id, I outer_limit) const {
        int index = 0;
        for (auto inter: positions_map) {
            I max_value = inter.second.first == max ? outer_limit : inter.second.first;
            if (inter.first <= patch_id && patch_id < max_value) {
                return std::unique_ptr<P>(new P(inter.second.second));
            }
        }
        return nullptr;
    }


public:
    explicit DVIntervalList() : max(std::numeric_limits<I>::max()),
                                patches(max),
                                local_changes(max) {}

    /**
     * Add new element to the list
     * @param element the new element
     */
    bool addition(const T& element) {
        I patch = element.get_patch_id();
        bool has_changed = patches.addition(patch);
        if (element.is_local_change()) {
            has_changed |= local_changes.addition(patch);
        }
        if (element.has_positions()) {
            has_changed |= insert_positions(element.get_patch_id(), element.get_patch_positions());
        }
        return has_changed;
    }

    bool deletion(I element) {
        bool has_changed = patches.deletion(element);
        has_changed |= local_changes.deletion(element);
        has_changed |= delete_positions(element);
        return has_changed;
    }

    long get_index(I patch_id, I outer_patch_limit) const {
        return patches.get_index(patch_id, outer_patch_limit);
    }

    long size(I outer_patch_limit) const {
        return patches.get_size(outer_patch_limit);
    }

    T get_element_at(long index, I outer_patch_limit) const {
        I patch_id = patches.get_element_at(index, outer_patch_limit);
        bool local_change = local_changes.is_in(patch_id);
        std::unique_ptr<P> positions = get_position(patch_id, outer_patch_limit);
        if (positions == nullptr) {
            return T(patch_id, local_change);
        }
        return T(patch_id, local_change, *positions);
    }

    std::pair<const char*, size_t> serialize() const {
        auto data_patches = patches.serialize();
        auto data_local = local_changes.serialize();
        size_t size = 2 * sizeof(size_t) + data_patches.second + data_local.second + (positions_map.size() * (2*sizeof(I) + sizeof(P)));
        char* data = new char[size];
        char* data_p = data;

        std::memcpy(data_p, &data_patches.second, sizeof(size_t));
        data_p += sizeof(size_t);

        std::memcpy(data_p, &data_local.second, sizeof(size_t));
        data_p += sizeof(size_t);

        std::memcpy(data_p, data_patches.first, data_patches.second);
        data_p += data_patches.second;
        delete[] data_patches.first;

        std::memcpy(data_p, data_local.first, data_local.second);
        data_p += data_local.second;
        delete[] data_local.first;

        for (const auto& inter_p: positions_map) {
            std::memcpy(data_p, &inter_p.first, sizeof(I));
            data_p += sizeof(I);
            std::memcpy(data_p, &inter_p.second.first, sizeof(I));
            data_p += sizeof(I);
            std::memcpy(data_p, &inter_p.second.second, sizeof(P));
            data_p += sizeof(P);
        }

        return std::make_pair(data, size);
    }

    void deserialize(const char *data, size_t size) {
        const char* data_p = data;
        size_t patches_size, local_size, position_size;
        std::memcpy(&patches_size, data_p, sizeof(size_t));
        data_p += sizeof(size_t);
        std::memcpy(&local_size, data_p, sizeof(size_t));
        data_p += sizeof(size_t);
        position_size = size - local_size - patches_size - 2*sizeof(size_t);

        patches.clear();
        local_changes.clear();
        positions_map.clear();

        patches.deserialize(data_p, patches_size);
        data_p += patches_size;
        local_changes.deserialize(data_p, local_size);
        data_p += local_size;

        auto it = positions_map.begin();
        for (size_t i = 0; i<position_size; i+=(2*sizeof(I) + sizeof(P))) {
            I s, e;
            std::memcpy(&s, data_p+i, sizeof(I));
            std::memcpy(&e, data_p+i+sizeof(I), sizeof(I));
            P p;
            std::memcpy(&p, data_p+i+2*sizeof(I), sizeof(P));
            positions_map[s] = std::make_pair(e, std::move(p));
            positions_map.emplace_hint(it, s, std::make_pair(e, std::move(p)));
            it = positions_map.end();
        }
    }

};



template<class T, class I>
class DVIntervalListV2 {
private:
    IntervalList<I> patches;
    IntervalList<I> local_changes;
    std::vector<PatchPositions> position_vec;


    std::pair<const char*, size_t> serialize_positions() const {
        auto position_diff = [](const PatchPositions& first, const PatchPositions& second) {
            uint8_t head = 0;
            std::vector<PatchPosition> diff_values;

            PatchPosition diff1 = second.sp_ - first.sp_;
            head |= (bool)diff1 << 0;
            if (diff1) diff_values.push_back(diff1);

            PatchPosition diff2 = second.s_o - first.s_o;
            head |= (bool)diff2 << 1;
            if (diff2) diff_values.push_back(diff2);

            PatchPosition diff3 = second.s__ - first.s__;
            head |= (bool)diff3 << 2;
            if (diff3) diff_values.push_back(diff3);

            PatchPosition diff4 = second._po - first._po;
            head |= (bool)diff4 << 3;
            if (diff4) diff_values.push_back(diff4);

            PatchPosition diff5 = second._p_ - first._p_;
            head |= (bool)diff5 << 4;
            if (diff5) diff_values.push_back(diff5);

            PatchPosition diff6 = second.__o - first.__o;
            head |= (bool)diff6 << 5;
            if (diff6) diff_values.push_back(diff6);

            PatchPosition diff7 = second.___ - first.___;
            head |= (bool)diff7 << 6;
            if (diff7) diff_values.push_back(diff7);

            size_t size = sizeof(uint8_t) + diff_values.size()*sizeof(PatchPosition);
            char* data = new char[size];
            std::memcpy(data, &head, sizeof(uint8_t));
            std::memcpy(data+sizeof(uint8_t), diff_values.data(), diff_values.size()*sizeof(PatchPosition));

            return std::make_pair(size, data);
        };
        char* data = nullptr;
        size_t size = 0;
        if (position_vec.size() == 1) {
            size = sizeof(PatchPositions);
            data = new char[size];
            std::memcpy(data, &position_vec[0], sizeof(PatchPositions));
        } else if (position_vec.size() > 1) { // if more than 1 PatchPositions, we use delta-encoding
            // We can't know before encoding the total size. PatchPositions + (>= 1 bytes)
            size_t total_size = sizeof(PatchPositions);
            std::vector<std::pair<size_t, char*>> data_vec;
            data_vec.reserve(position_vec.size());
            for (size_t i=1; i<position_vec.size(); i++) {  // Loop to perform delta encoding (i-1 & ith elements)
                auto d = position_diff(position_vec[i-1], position_vec[i]);
                total_size += d.first;
                data_vec.push_back(d);
            }
            // We allocate the correct amount of memory
            size = total_size;
            data = new char[total_size];
            std::memcpy(data, &position_vec[0], sizeof(PatchPositions)); // plain copy of 1st PatchPositions
            char* data_p = data + sizeof(PatchPositions);
            for (auto& p: data_vec) {  // loop to copy the "delta" PatchPositions
                std::memcpy(data_p, p.second, p.first);
                data_p += p.first;
            }
        }
        return std::make_pair(data, size);
    }

    void deserialize_positions(const char* data, size_t size) {
        auto position_undiff = [] (const char* data, const PatchPositions& prev, PatchPositions& pos) {
            uint8_t head;
            size_t offset = 0;
            std::memcpy(&head, data+offset, sizeof(uint8_t));
            offset += sizeof(uint8_t);
            PatchPosition diff;
            if (head & 1) {
                std::memcpy(&diff, data+offset, sizeof(PatchPosition));
                pos.sp_ = prev.sp_ + diff;
                offset += sizeof(PatchPosition);
            }
            head >>= 1;
            if (head & 1) {
                std::memcpy(&diff, data+offset, sizeof(PatchPosition));
                pos.s_o = prev.s_o + diff;
                offset += sizeof(PatchPosition);
            }
            head >>= 1;
            if (head & 1) {
                std::memcpy(&diff, data+offset, sizeof(PatchPosition));
                pos.s__ = prev.s__ + diff;
                offset += sizeof(PatchPosition);
            }
            head >>= 1;
            if (head & 1) {
                std::memcpy(&diff, data+offset, sizeof(PatchPosition));
                pos._po = prev._po + diff;
                offset += sizeof(PatchPosition);
            }
            head >>= 1;
            if (head & 1) {
                std::memcpy(&diff, data+offset, sizeof(PatchPosition));
                pos._p_ = prev._p_ + diff;
                offset += sizeof(PatchPosition);
            }
            head >>= 1;
            if (head & 1) {
                std::memcpy(&diff, data+offset, sizeof(PatchPosition));
                pos.__o = prev.__o + diff;
                offset += sizeof(PatchPosition);
            }
            head >>= 1;
            if (head & 1) {
                std::memcpy(&diff, data+offset, sizeof(PatchPosition));
                pos.___ = prev.___ + diff;
                offset += sizeof(PatchPosition);
            }
            return offset;
        };

        position_vec.resize(1);
        std::memcpy(&position_vec[0], data, sizeof(PatchPositions));
        const char* data_p = data + sizeof(PatchPositions);
        PatchPositions& prev = position_vec[0];
        while(data_p != data+size) {
            position_vec.emplace_back();
            data_p += position_undiff(data_p, prev, position_vec.back());
            prev = position_vec.back();
        }
    }

public:
    explicit DVIntervalListV2() : patches(std::numeric_limits<I>::max()),
                                  local_changes(std::numeric_limits<I>::max()) {}

    /**
     * Add new element to the list
     * @param element the new element
     */
    bool addition(const T& element, I outer_patch_limit) {
        I patch = element.get_patch_id();
        bool has_changed = patches.addition(patch);
        if (element.is_local_change()) {
            has_changed |= local_changes.addition(patch);
        }
        if (element.has_positions()) {
            has_changed = true;
            long index = patches.get_index(patch, outer_patch_limit);
            if (index >= position_vec.size()) {
                position_vec.emplace_back(element.get_patch_positions());
            } else {
                position_vec[index] = element.get_patch_positions();
            }
        }
        return has_changed;
    }

    bool deletion(I element, I outer_patch_limit) {
        bool has_changed = false;
        if (!position_vec.empty()) {
            has_changed = true;
            long index = patches.get_index(element, outer_patch_limit);
            if (index < position_vec.size() && index >= 0)
                position_vec.erase(std::next(position_vec.begin(), index));
        }
        has_changed |= patches.deletion(element);
        has_changed |= local_changes.deletion(element);
        return has_changed;
    }

    long get_index(I patch_id, I outer_patch_limit) const {
        return patches.get_index(patch_id, outer_patch_limit);
    }

    long size(I outer_patch_limit) const {
        return patches.get_size(outer_patch_limit);
    }

    T get_element_at(long index, I outer_patch_limit) const {
        I patch_id = patches.get_element_at(index, outer_patch_limit);
        bool local_change = local_changes.is_in(patch_id);
        if (position_vec.empty()) {
            return T(patch_id, local_change);
        }
        return T(patch_id, local_change, position_vec[index]);
    }

    std::pair<const char*, size_t> serialize() const {
        auto data_patches = patches.serialize();
        auto data_local = local_changes.serialize();
        auto data_positions = serialize_positions();
        size_t size = 2 * sizeof(size_t) + data_patches.second + data_local.second + data_positions.second;
        char* data = new char[size];
        char* data_p = data;

        std::memcpy(data_p, &data_patches.second, sizeof(size_t));
        data_p += sizeof(size_t);

        std::memcpy(data_p, &data_local.second, sizeof(size_t));
        data_p += sizeof(size_t);

        std::memcpy(data_p, data_patches.first, data_patches.second);
        data_p += data_patches.second;
        delete[] data_patches.first;

        std::memcpy(data_p, data_local.first, data_local.second);
        data_p += data_local.second;
        delete[] data_local.first;

        std::memcpy(data_p, data_positions.first, data_positions.second);
        delete[] data_positions.first;

        return std::make_pair(data, size);
    }

    void deserialize(const char *data, size_t size) {
        const char* data_p = data;
        size_t patches_size, local_size, position_size;
        std::memcpy(&patches_size, data_p, sizeof(size_t));
        data_p += sizeof(size_t);
        std::memcpy(&local_size, data_p, sizeof(size_t));
        data_p += sizeof(size_t);
        position_size = size - local_size - patches_size - 2*sizeof(size_t);

        patches.clear();
        local_changes.clear();

        patches.deserialize(data_p, patches_size);
        data_p += patches_size;
        local_changes.deserialize(data_p, local_size);
        data_p += local_size;
        deserialize_positions(data_p, position_size);
    }

};


template <class I>
struct VerPatchPositions {
    I patch_id_start;
    PatchPositions positions;

    bool operator<(const VerPatchPositions &rhs) const {
        return patch_id_start < rhs.patch_id_start;
    }

    bool operator>(const VerPatchPositions &rhs) const {
        return patch_id_start > rhs.patch_id_start;
    }

};

template<class T, class I>
class DVIntervalListV3 {
private:
    I max;
    IntervalList<I> patches;
    IntervalList<I> local_changes;
    std::vector<VerPatchPositions<I>> position_vec;


    bool insert_positions(I patch_id, PatchPositions positions) {
        bool has_changed = false;
        VerPatchPositions<I> verpos = {patch_id, positions};
        if (position_vec.empty()) {
            position_vec.push_back(verpos);
            has_changed = true;
        } else {
            auto it_insert = std::lower_bound(position_vec.begin(), position_vec.end(), verpos);
            if (it_insert == position_vec.end() || *it_insert > verpos) it_insert--;
            if ((*it_insert).patch_id_start == verpos.patch_id_start) {  // patch_id equal
                *it_insert = verpos;  // we overwrite
                has_changed = true;
            } else if ((*it_insert).positions != verpos.positions) {  // it_insert < verpos -> we have new PatchPositions, so we insert
//                VerPatchPositions<I> verpos_n = *it_insert;
                it_insert = position_vec.insert(++it_insert, verpos);
//                position_vec.insert(++it_insert, {patch_id + 1, verpos_n.positions});
                has_changed = true;
            } // else PatchPositions are equal we don't need to do anything
        }
        return has_changed;
    }

    bool get_positions(I patch_id, PatchPositions& positions) const {
        VerPatchPositions<I> verpos = {patch_id, PatchPositions()};  // temporary for search
        auto pos = std::lower_bound(position_vec.begin(), position_vec.end(), verpos);
        if (pos == position_vec.end() || *pos > verpos) pos--;
        if (pos == position_vec.begin()) return false;
        positions = (*pos).positions;
        return true;
    }

    std::pair<const char*, size_t> serialize_positions() const {
        auto position_diff = [](const VerPatchPositions<I>& first, const VerPatchPositions<I>& second) {
            uint8_t head = 0;
            std::vector<PatchPosition> diff_values;

            PatchPosition diff1 = second.positions.sp_ - first.positions.sp_;
            head |= (bool)diff1 << 0;
            if (diff1) diff_values.push_back(diff1);

            PatchPosition diff2 = second.positions.s_o - first.positions.s_o;
            head |= (bool)diff2 << 1;
            if (diff2) diff_values.push_back(diff2);

            PatchPosition diff3 = second.positions.s__ - first.positions.s__;
            head |= (bool)diff3 << 2;
            if (diff3) diff_values.push_back(diff3);

            PatchPosition diff4 = second.positions._po - first.positions._po;
            head |= (bool)diff4 << 3;
            if (diff4) diff_values.push_back(diff4);

            PatchPosition diff5 = second.positions._p_ - first.positions._p_;
            head |= (bool)diff5 << 4;
            if (diff5) diff_values.push_back(diff5);

            PatchPosition diff6 = second.positions.__o - first.positions.__o;
            head |= (bool)diff6 << 5;
            if (diff6) diff_values.push_back(diff6);

            PatchPosition diff7 = second.positions.___ - first.positions.___;
            head |= (bool)diff7 << 6;
            if (diff7) diff_values.push_back(diff7);

            size_t size = sizeof(uint8_t) + sizeof(I) + diff_values.size()*sizeof(PatchPosition);
            char* data = new char[size];
            std::memcpy(data, &head, sizeof(uint8_t));
            std::memcpy(data+sizeof(uint8_t), &second.patch_id_start, sizeof(PatchPosition));
            std::memcpy(data+sizeof(uint8_t)+sizeof(I), diff_values.data(), diff_values.size()*sizeof(PatchPosition));

            return std::make_pair(size, data);
        };

        char* data = nullptr;
        size_t size = 0;
        if (position_vec.size() == 1) {
            size = sizeof(PatchPositions) + sizeof(I);
            data = new char[size];
            std::memcpy(data, &position_vec[0].patch_id_start, sizeof(I));
            std::memcpy(data+sizeof(I), &position_vec[0].positions, sizeof(PatchPositions));
        } else if (position_vec.size() > 1) { // if more than 1 PatchPositions, we use delta-encoding
            // We can't know before encoding the total size. PatchPositions + (>= 1 bytes)
            size_t total_size = sizeof(PatchPositions) + sizeof(I);
            std::vector<std::pair<size_t, char*>> data_vec;
            data_vec.reserve(position_vec.size());
            for (size_t i=1; i<position_vec.size(); i++) {  // Loop to perform delta encoding (i-1 & ith elements)
                auto d = position_diff(position_vec[i-1], position_vec[i]);
                total_size += d.first;
                data_vec.push_back(d);
            }
            // We allocate the correct amount of memory
            size = total_size;
            data = new char[total_size];
            std::memcpy(data, &position_vec[0].patch_id_start, sizeof(I));
            std::memcpy(data+sizeof(I), &position_vec[0].positions, sizeof(PatchPositions)); // plain copy of 1st PatchPositions
            char* data_p = data + sizeof(I) + sizeof(PatchPositions) ;
            for (auto& p: data_vec) {  // loop to copy the "delta" PatchPositions
                std::memcpy(data_p, p.second, p.first);
                data_p += p.first;
            }
        }
        return std::make_pair(data, size);
    }

    void deserialize_positions(const char* data, size_t size) {
        auto position_undiff = [] (const char* data, const PatchPositions& prev, PatchPositions& pos) {
            uint8_t head;
            size_t offset = 0;
            std::memcpy(&head, data+offset, sizeof(uint8_t));
            offset += sizeof(uint8_t);
            PatchPosition diff;
            if (head & 1) {
                std::memcpy(&diff, data+offset, sizeof(PatchPosition));
                pos.sp_ = prev.sp_ + diff;
                offset += sizeof(PatchPosition);
            }
            head >>= 1;
            if (head & 1) {
                std::memcpy(&diff, data+offset, sizeof(PatchPosition));
                pos.s_o = prev.s_o + diff;
                offset += sizeof(PatchPosition);
            }
            head >>= 1;
            if (head & 1) {
                std::memcpy(&diff, data+offset, sizeof(PatchPosition));
                pos.s__ = prev.s__ + diff;
                offset += sizeof(PatchPosition);
            }
            head >>= 1;
            if (head & 1) {
                std::memcpy(&diff, data+offset, sizeof(PatchPosition));
                pos._po = prev._po + diff;
                offset += sizeof(PatchPosition);
            }
            head >>= 1;
            if (head & 1) {
                std::memcpy(&diff, data+offset, sizeof(PatchPosition));
                pos._p_ = prev._p_ + diff;
                offset += sizeof(PatchPosition);
            }
            head >>= 1;
            if (head & 1) {
                std::memcpy(&diff, data+offset, sizeof(PatchPosition));
                pos.__o = prev.__o + diff;
                offset += sizeof(PatchPosition);
            }
            head >>= 1;
            if (head & 1) {
                std::memcpy(&diff, data+offset, sizeof(PatchPosition));
                pos.___ = prev.___ + diff;
                offset += sizeof(PatchPosition);
            }
            return offset;
        };

        position_vec.resize(1);
        std::memcpy(&position_vec[0].patch_id_start, data, sizeof(I));
        std::memcpy(&position_vec[0].positions, data+sizeof(I), sizeof(PatchPositions));
//        const char* data_p = data + sizeof(I) + sizeof(PatchPositions);
        size_t offset = sizeof(I) + sizeof(PatchPositions);
        PatchPositions& prev = position_vec[0].positions;
        while(offset < size) {
            position_vec.emplace_back();  // insert default value at the back the vector to be modified
            offset += position_undiff(data+offset, prev, position_vec.back().positions);
            prev = position_vec.back().positions;
        }
    }

public:
    explicit DVIntervalListV3() : max(std::numeric_limits<I>::max()),
                                  patches(max),
                                  local_changes(max) {}

    /**
     * Add new element to the list
     * @param element the new element
     */
    bool addition(const T& element) {
        I patch = element.get_patch_id();
        bool has_changed = patches.addition(patch);
        if (element.is_local_change()) {
            has_changed |= local_changes.addition(patch);
        }
        if (element.has_positions()) {
            has_changed |= insert_positions(patch, element.get_patch_positions());
        }
        return has_changed;
    }

    bool deletion(I element, I outer_patch_limit) {
        bool has_changed = false;
        has_changed |= patches.deletion(element);
        has_changed |= local_changes.deletion(element);
        return has_changed;
    }

    long get_index(I patch_id, I outer_patch_limit) const {
        return patches.get_index(patch_id, outer_patch_limit);
    }

    long size(I outer_patch_limit) const {
        return patches.get_size(outer_patch_limit);
    }

    T get_element_at(long index, I outer_patch_limit) const {
        I patch_id = patches.get_element_at(index, outer_patch_limit);
        bool local_change = local_changes.is_in(patch_id);
        if (position_vec.empty()) {
            return T(patch_id, local_change);
        }
        PatchPositions p;
        get_positions(patch_id, p);
        return T(patch_id, local_change, p);
    }

    std::pair<const char*, size_t> serialize() const {
        auto data_patches = patches.serialize();
        auto data_local = local_changes.serialize();
        auto data_positions = serialize_positions();
        size_t size = 2 * sizeof(size_t) + data_patches.second + data_local.second + data_positions.second;
        char* data = new char[size];
        char* data_p = data;

        std::memcpy(data_p, &data_patches.second, sizeof(size_t));
        data_p += sizeof(size_t);

        std::memcpy(data_p, &data_local.second, sizeof(size_t));
        data_p += sizeof(size_t);

        std::memcpy(data_p, data_patches.first, data_patches.second);
        data_p += data_patches.second;
        delete[] data_patches.first;

        std::memcpy(data_p, data_local.first, data_local.second);
        data_p += data_local.second;
        delete[] data_local.first;

        std::memcpy(data_p, data_positions.first, data_positions.second);
        delete[] data_positions.first;

        return std::make_pair(data, size);
    }

    void deserialize(const char *data, size_t size) {
        const char* data_p = data;
        size_t patches_size, local_size, position_size;
        std::memcpy(&patches_size, data_p, sizeof(size_t));
        data_p += sizeof(size_t);
        std::memcpy(&local_size, data_p, sizeof(size_t));
        data_p += sizeof(size_t);
        position_size = size - local_size - patches_size - 2*sizeof(size_t);

        patches.clear();
        local_changes.clear();

        patches.deserialize(data_p, patches_size);
        data_p += patches_size;
        local_changes.deserialize(data_p, local_size);
        data_p += local_size;
        deserialize_positions(data_p, position_size);
    }

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

template <class T>
class PatchTreeDeletionValueBase {
protected:
    DVIntervalList<T, int, PatchPositions> elements;
    int max_patch_id;
public:
    explicit PatchTreeDeletionValueBase(int max_patch_id);
    /**
     * Add the given element.
     * @param element The value element to add
     */
    bool add(const T& element);
    bool del(int patch_id);
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
    T get_patch(long element) const;
    /**
     * @param patch_id The patch id
     * @return The patch.
     */
    T get(int patch_id) const;
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
