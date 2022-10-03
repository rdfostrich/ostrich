#ifndef TPFPATCH_STORE_PATCH_TREE_DELETION_VALUE_H
#define TPFPATCH_STORE_PATCH_TREE_DELETION_VALUE_H

#include <string>
#include <cstddef>
#include <vector>
#include <memory>
#include "triple.h"
#include "interval_list.h"
#include "variable_size_integer.h"

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

    char* serialize(size_t* size) const {
        *size = 0;
        char* data = new char[max_serialization_size()];
#ifdef USE_VSI
        std::vector<uint8_t> buffer;

        encode_SLEB128<PatchPosition>(this->sp_, buffer);
        std::memcpy(data+*size, buffer.data(), buffer.size());
        *size += buffer.size();
        buffer.clear();

        encode_SLEB128<PatchPosition>(this->s_o, buffer);
        std::memcpy(data+*size, buffer.data(), buffer.size());
        *size += buffer.size();
        buffer.clear();

        encode_SLEB128<PatchPosition>(this->s__, buffer);
        std::memcpy(data+*size, buffer.data(), buffer.size());
        *size += buffer.size();
        buffer.clear();

        encode_SLEB128<PatchPosition>(this->_po, buffer);
        std::memcpy(data+*size, buffer.data(), buffer.size());
        *size += buffer.size();
        buffer.clear();

        encode_SLEB128<PatchPosition>(this->_p_, buffer);
        std::memcpy(data+*size, buffer.data(), buffer.size());
        *size += buffer.size();
        buffer.clear();

        encode_SLEB128<PatchPosition>(this->__o, buffer);
        std::memcpy(data+*size, buffer.data(), buffer.size());
        *size += buffer.size();
        buffer.clear();

        encode_SLEB128<PatchPosition>(this->___, buffer);
        std::memcpy(data+*size, buffer.data(), buffer.size());
        *size += buffer.size();
#else
        std::memcpy(data, this, sizeof(PatchPositions));
        *size = sizeof(PatchPositions);
#endif
        return data;
    }

    size_t deserialize(const char* data) {
#ifdef USE_VSI
        size_t decode_size = 0;
        size_t offset = 0;
        this->sp_ = decode_SLEB128<PatchPosition>((const uint8_t*)data, &decode_size);
        offset += decode_size;

        this->s_o = decode_SLEB128<PatchPosition>((const uint8_t*)data+offset, &decode_size);
        offset += decode_size;

        this->s__ = decode_SLEB128<PatchPosition>((const uint8_t*)data+offset, &decode_size);
        offset += decode_size;

        this->_po = decode_SLEB128<PatchPosition>((const uint8_t*)data+offset, &decode_size);
        offset += decode_size;

        this->_p_ = decode_SLEB128<PatchPosition>((const uint8_t*)data+offset, &decode_size);
        offset += decode_size;

        this->__o = decode_SLEB128<PatchPosition>((const uint8_t*)data+offset, &decode_size);
        offset += decode_size;

        this->___ = decode_SLEB128<PatchPosition>((const uint8_t*)data+offset);
        offset += decode_size;
        return offset;
#else
        std::memcpy(this, data, sizeof(PatchPositions));
        return sizeof(PatchPositions);
#endif
    }

    static size_t max_serialization_size() {
        PatchPosition max = std::numeric_limits<PatchPosition>::max();
        size_t bytes = get_SLEB128_size<PatchPosition>(max);
        return 7 * bytes;
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

    virtual char* serialize(size_t* size) const;
    virtual size_t deserialize(const char* data);
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

    char* serialize(size_t* size) const override;
    size_t deserialize(const char* data) override;
};


template <class I>
struct VerPatchPositions {
    I patch_id;
    PatchPositions positions;

    bool operator<(const VerPatchPositions &rhs) const {
        return patch_id < rhs.patch_id;
    }

    bool operator>(const VerPatchPositions &rhs) const {
        return patch_id > rhs.patch_id;
    }
};


class PatchPositionsContainer {
public:
    virtual bool insert_positions(int patch_id, const PatchPositions& positions) = 0;
    virtual bool delete_positions(int patch_id) = 0;
    virtual bool get_positions(int patch_id, PatchPositions& positions, int outer_patch_limit) const = 0;

    virtual std::pair<const char*, size_t> serialize() const = 0;
    virtual void deserialize(const char* data, size_t size) = 0;
};


class IntervalPatchPositionsContainer: public PatchPositionsContainer {
private:
    int max;
    std::map<int, std::pair<int, PatchPositions>> positions_map;

public:
    IntervalPatchPositionsContainer() : max(std::numeric_limits<int>::max()) {}

    bool insert_positions(int patch_id, const PatchPositions& positions) override;
    bool delete_positions(int patch_id) override;
    bool get_positions(int patch_id, PatchPositions& positions, int outer_patch_limit) const override;

    std::pair<const char*, size_t> serialize() const override;
    void deserialize(const char* data, size_t size) override;
};


class DeltaPatchPositionsContainerBase: public PatchPositionsContainer {
protected:
    std::vector<VerPatchPositions<int>> position_vec;

    static inline size_t decode_diff(const char *data, PatchPosition &diff);

    static inline void delta_serialize_position_vec(const std::vector<VerPatchPositions<int>>& position_vec, char** data, size_t* size);
    static inline void delta_deserialize_position_vec(std::vector<VerPatchPositions<int>>& position_vec, const char* data, size_t size);
    static inline void serialize_position_vec(const std::vector<VerPatchPositions<int>>& position_vec, char** data, size_t* size);
    static inline void deserialize_position_vec(std::vector<VerPatchPositions<int>>& position_vec, const char* data, size_t size);

public:
    bool insert_positions(int patch_id, const PatchPositions& positions) override = 0;
    bool delete_positions(int patch_id) override = 0;
    bool get_positions(int patch_id, PatchPositions& positions, int outer_patch_limit) const override = 0;

    std::pair<const char*, size_t> serialize() const override = 0;
    void deserialize(const char* data, size_t size) override = 0;
};

class DeltaPatchPositionsContainer: public DeltaPatchPositionsContainerBase {
public:
    bool insert_positions(int patch_id, const PatchPositions& positions) override;
    bool delete_positions(int patch_id) override;
    bool get_positions(int patch_id, PatchPositions& positions, int outer_patch_limit) const override;

    std::pair<const char*, size_t> serialize() const override;
    void deserialize(const char* data, size_t size) override;
};

class DeltaPatchPositionsContainerV2: public DeltaPatchPositionsContainerBase {
public:
    bool insert_positions(int patch_id, const PatchPositions& positions) override;
    bool delete_positions(int patch_id) override;
    bool get_positions(int patch_id, PatchPositions& positions, int outer_patch_limit) const override;

    std::pair<const char*, size_t> serialize() const override;
    void deserialize(const char* data, size_t size) override;
};


/**
 * Specialized class to handle DeletionValueElements as intervals
 * Ordering is based on the patch_id.
 * Equality is based on internals (i.e. local_change flag and patch_positions)
 *
 * @tparam T DeletionValueElement type
 */
template <class T>
class DVIntervalList {
private:
    IntervalList<int> patches;
    IntervalList<int> local_changes;
    std::unique_ptr<PatchPositionsContainer> patch_positions_container;

public:
    DVIntervalList() : patches(std::numeric_limits<int>::max()),
                       local_changes(std::numeric_limits<int>::max()),
                       patch_positions_container(new DeltaPatchPositionsContainerV2) {}

    bool addition(const T& element) {
        int patch = element.get_patch_id();
        bool has_changed = patches.addition(patch);
        if (element.is_local_change()) {
            has_changed |= local_changes.addition(patch);
        }
        if (element.has_positions()) {
            has_changed |= patch_positions_container->insert_positions(patch, element.get_patch_positions());
        }
        return has_changed;
    }

    bool deletion(int element) {
        bool has_changed = false;
        has_changed |= patches.deletion(element);
        has_changed |= local_changes.deletion(element);
        has_changed |= patch_positions_container->delete_positions(element);
        return has_changed;
    }

    long get_index(int patch_id, int outer_patch_limit) const {
        return patches.get_index(patch_id, outer_patch_limit);
    }

    long size(int outer_patch_limit) const {
        return patches.get_size(outer_patch_limit);
    }

    T get_element_at(long index, int outer_patch_limit) const {
        int patch_id = patches.get_element_at(index, outer_patch_limit);
        bool local_change = local_changes.is_in(patch_id);
        if (!T::has_positions()) {
            return T(patch_id, local_change);
        }
        PatchPositions p;
        patch_positions_container->get_positions(patch_id, p, outer_patch_limit);
        return T(patch_id, local_change, p);
    }

    std::pair<const char*, size_t> serialize() const {
        auto data_patches = patches.serialize();
        auto data_local = local_changes.serialize();
        auto data_positions = patch_positions_container->serialize();
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
        if (position_size > 0) {
            patch_positions_container->deserialize(data_p, position_size);
        }
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
    DVIntervalList<T> elements;
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
