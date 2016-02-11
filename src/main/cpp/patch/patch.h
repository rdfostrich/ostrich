#ifndef TPFPATCH_STORE_PATCH_H
#define TPFPATCH_STORE_PATCH_H

#include <string>
#include "triple.h"

using namespace std;

// A key in the PatchTree is a triple
typedef Triple PatchTreeKey;

// A value in the PatchTree is a linked list of patch_id->patch_position
// This can alternatively be implemented as a map for improving efficiency
typedef struct PatchTreeValue {
    int patch_id;
    int patch_position;
    bool addition;
    bool next;
    PatchTreeValue(int patch_id, int patch_position, bool addition, bool next) :
            patch_id(patch_id), patch_position(patch_position), addition(addition), next(next) {}
} PatchTreeValue;

#endif //TPFPATCH_STORE_PATCH_H
