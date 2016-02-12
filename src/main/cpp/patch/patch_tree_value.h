#ifndef TPFPATCH_STORE_PATCH_TREE_VALUE_H
#define TPFPATCH_STORE_PATCH_TREE_VALUE_H

#include <string>
#include <cstddef>
#include <vector>

using namespace std;

typedef long PatchPosition; // TODO: Maybe we can convert this to an int, to reduce tree size

// A PatchTreeValueElement contains a patch id, a relative patch position and
// an indication whether or not this is an addition or deletion.
class PatchTreeValueElement {
protected:
    int patch_id;
    PatchPosition patch_position;
    bool addition;
public:
    int get_patch_id();
    PatchPosition get_patch_position();
    bool is_addition();
    PatchTreeValueElement() : patch_id(-1), patch_position(-1), addition(false) {} // Required for vector#resize
    PatchTreeValueElement(int patch_id, long patch_position, bool addition) :
            patch_id(patch_id), patch_position(patch_position), addition(addition) {}
    bool operator < (const PatchTreeValueElement &rhs) const { return patch_id < rhs.patch_id; }
};

// A PatchTreeValue in a PatchTree is a sorted list of PatchTreeValueElements
// It contains the information corresponding to one triple in the patch tree.
// It can be seen as a mapping from patch id to the pair of relative patch
// position and element type (addition or deletion).
class PatchTreeValue {
protected:
    std::vector<PatchTreeValueElement> elements;
public:
    PatchTreeValue();
    /**
     * Add the given element.
     * @param element The value element to add
     */
    void add(PatchTreeValueElement element);
    /**
     * Get the index of the given patch in this value list.
     * @param patch_id The id of the patch to find
     * @return The index of the given patch in this value list. -1 if not found.
     */
    long get_patchvalue_index(int patch_id);
    /**
     * Get the patch of the given element.
     * @param element The element index in this value list. This can be the result of get_patchvalue_index().
     * @return The patch.
     */
    PatchTreeValueElement get_patch(long element);
    /**
     * @param patch_id The patch id
     * @return The patch.
     */
    PatchTreeValueElement get(int patch_id);
    /**
     * @return The string representation of this patch.
     */
    string to_string();
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


#endif //TPFPATCH_STORE_PATCH_TREE_VALUE_H
