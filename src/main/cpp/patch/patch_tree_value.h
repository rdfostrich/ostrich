#ifndef TPFPATCH_STORE_PATCH_TREE_VALUE_H
#define TPFPATCH_STORE_PATCH_TREE_VALUE_H

#include <string>
#include <cstddef>
#include <vector>

using namespace std;

class PatchTreeValueElement {
protected:
    int patch_id;
    int patch_position;
    bool addition;
public:
    int get_patch_id();
    int get_patch_position();
    bool is_addition();
    PatchTreeValueElement() : patch_id(-1), patch_position(-1), addition(false) {} // Required for vector#resize
    PatchTreeValueElement(int patch_id, int patch_position, bool addition) :
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
    bool contains(int patch_id);
    PatchTreeValueElement get(int patch_id);
    string to_string();
    const char* serialize(size_t* size);
    void deserialize(const char* data, size_t size);
};


#endif //TPFPATCH_STORE_PATCH_TREE_VALUE_H
