#ifndef TPFPATCH_STORE_PATCH_TREE_ADDITION_VALUE_H
#define TPFPATCH_STORE_PATCH_TREE_ADDITION_VALUE_H

#include <vector>

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


#endif //TPFPATCH_STORE_PATCH_TREE_ADDITION_VALUE_H
