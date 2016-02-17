#ifndef TPFPATCH_STORE_PATCH_TREE_VALUE_H
#define TPFPATCH_STORE_PATCH_TREE_VALUE_H

#include <string>
#include <cstddef>
#include <vector>

using namespace std;

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
    string to_string() {
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
} PatchPositions;

// A PatchTreeValueElement contains a patch id, a relative patch position and
// an indication whether or not this is an addition or deletion.
class PatchTreeValueElement {
protected:
    int patch_id;
    PatchPositions patch_positions;
    bool addition;
public:
    int get_patch_id();
    PatchPositions get_patch_positions();
    bool is_addition();
    PatchTreeValueElement() : patch_id(-1), patch_positions(PatchPositions()), addition(false) {} // Required for vector#resize
    PatchTreeValueElement(int patch_id, PatchPositions patch_positions, bool addition) :
            patch_id(patch_id), patch_positions(patch_positions), addition(addition) {}
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
     * @return The number of PatchTreeValueElement's stored in this value.
     */
    long get_size();
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
     * Check if this element represents an addition in the given patch id.
     * @param patch_id The patch id
     * @return If it is an addition
     */
    bool is_addition(int patch_id);
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
