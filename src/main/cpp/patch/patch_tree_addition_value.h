#ifndef TPFPATCH_STORE_PATCH_TREE_ADDITION_VALUE_H
#define TPFPATCH_STORE_PATCH_TREE_ADDITION_VALUE_H

#include <vector>
#include <string>
#include <cstddef>
#include "interval_list.h"

#define COMPRESSED_TREE_VALUES

#ifndef COMPRESSED_TREE_VALUES
class PatchTreeAdditionValue {
protected:
    std::vector<int> patches;
    std::vector<int> local_changes;
public:
    PatchTreeAdditionValue();
    /**
     * Add the given patch id.
     * @param patch_id The patch id to add
     */
    void add(int patch_id);
    /**
     * Check if the given patch id is present in this value.
     * @param patch_id The id of the patch to find
     * @return If the given patch id is present
     */
    bool is_patch_id(int patch_id) const;
    /**
     * Get the index of the given patch in this value list.
     * @param patch_id The id of the patch to find
     * @return The index of the given patch in this value list. -1 if not found.
     */
    long get_patchvalue_index(int patch_id) const;
    /**
     * Get a patch id by position.
     * @param i The index to check the patch id for
     * @return The patch id.
     */
    int get_patch_id_at(long i) const;
    /**
     * @return The number of PatchTreeDeletionValueElement's stored in this value.
     */
    long get_size() const;
    /**
     * Mark the given patch element as being a local change.
     * @param patch_id The id of the patch to set
     */
    void set_local_change(int patch_id);
    /**
     * Check if this element is an element (+) relative to the given patch itself,
     * For example in the series [t1- t1+ t1-], the element at index 1 is a local change,
     * while the others are global changes (with respect to the snapshot).
     * @param patch_id The id of the patch to get
     * @return If it is a local change.
     */
    bool is_local_change(int patch_id) const;
    /**
     * @return The string representation of this value.
     */
    std::string to_string() const;
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
};
#else
class PatchTreeAdditionValue {
protected:
    IntervalList<int> patches;
    IntervalList<int> local_changes;
    int max_patch_id;
public:
//    PatchTreeAdditionValue();
    /**
     * Construct a new AdditionValue with a specified max id.
     * This is needed in order to conduct correct get_size and get_patch_id_at operations
     * This is due to the fact that the last interval is stored unbound, so we need to know where it actually stops.
     * @param max_patch_id the outer bound of the values stored
     */
    explicit PatchTreeAdditionValue(int max_patch_id);
    /**
     * Add the given patch id.
     * @param patch_id The patch id to add
     * @return true if the internal representation changed (thus needing a disk rewrite)
     */
    bool add(int patch_id);
    /**
     * Add the given patch id as a standalone value.
     * @param patch_id
     * @return true if the internal representation changed (thus needing a disk rewrite)
     */
    bool add_unique(int patch_id);
    /**
     * Mark the patch id as a deletion. Mark the end of an interval in the internal representation.
     * @param patch_id the patch id to mark as deleted
     * @return true if the internal representation changed (thus needing a disk rewrite)
     */
    bool del(int patch_id);
    /**
     * Check if the given patch id is present in this value.
     * @param patch_id The id of the patch to find
     * @return If the given patch id is present
     */
    bool is_patch_id(int patch_id) const;
    /**
     * Get the index of the given patch in this value list.
     * @param patch_id The id of the patch to find
     * @return The index of the given patch in this value list. -1 if not found.
     */
    long get_patchvalue_index(int patch_id) const;
    /**
     * Get a patch id by position.
     * @param i The index to check the patch id for
     * @return The patch id.
     */
    int get_patch_id_at(long i) const;
    /**
     * @return The number of PatchTreeDeletionValueElement's stored in this value.
     */
    long get_size() const;
    /**
     * Mark the given patch element as being a local change.
     * @param patch_id The id of the patch to set
     */
    bool set_local_change(int patch_id);
    bool set_local_change_unique(int patch_id);
    bool unset_local_change(int patch_id);
    /**
     * Check if this element is an element (+) relative to the given patch itself,
     * For example in the series [t1- t1+ t1-], the element at index 1 is a local change,
     * while the others are global changes (with respect to the snapshot).
     * @param patch_id The id of the patch to get
     * @return If it is a local change.
     */
    bool is_local_change(int patch_id) const;
    /**
     * @return The string representation of this value.
     */
    std::string to_string() const;
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
};
#endif

#endif //TPFPATCH_STORE_PATCH_TREE_ADDITION_VALUE_H
