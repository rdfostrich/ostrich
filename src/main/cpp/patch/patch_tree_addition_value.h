#ifndef TPFPATCH_STORE_PATCH_TREE_ADDITION_VALUE_H
#define TPFPATCH_STORE_PATCH_TREE_ADDITION_VALUE_H

#include <vector>

class PatchTreeAdditionValue {
protected:
    std::vector<int> patches;
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
    bool is_patch_id(int patch_id);
    /**
     * @return The number of PatchTreeValueElement's stored in this value.
     */
    long get_size();
    /**
     * @return The string representation of this value.
     */
    std::string to_string();
    /**
     * Serialize this value to a byte array
     * @param size This will contain the size of the returned byte array
     * @return The byte array
     */
    const char* serialize(size_t* size);
    /**
     * Deserialize the given byte array to this object.
     * @param data The data to deserialize from.
     * @param size The size of the byte array
     */
    void deserialize(const char* data, size_t size);
};


#endif //TPFPATCH_STORE_PATCH_TREE_ADDITION_VALUE_H
