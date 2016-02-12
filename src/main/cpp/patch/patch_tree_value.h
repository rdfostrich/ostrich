#ifndef TPFPATCH_STORE_PATCH_TREE_VALUE_H
#define TPFPATCH_STORE_PATCH_TREE_VALUE_H

#include <string>
#include <cstddef>
#include <vector>

using namespace std;

typedef long PatchPosition; // TODO: Maybe we can convert this to an int, to reduce tree size

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

// A value in the PatchTree is a linked list of PatchTreeValueElements
class PatchTreeValue {
protected:
    std::vector<PatchTreeValueElement> elements;
public:
    PatchTreeValue();
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
    string to_string();
    const char* serialize(size_t* size);
    void deserialize(const char* data, size_t size);
};


#endif //TPFPATCH_STORE_PATCH_TREE_VALUE_H
