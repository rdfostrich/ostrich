#include <cstring>
#include <string>
#include <stdlib.h>
#include <iostream>
#include "patch_tree_addition_value.h"

PatchTreeAdditionValue::PatchTreeAdditionValue() : patches() {}


void PatchTreeAdditionValue::add(int patch_id) {
    std::vector<int>::iterator itToInsert = std::lower_bound(
            patches.begin(), patches.end(), patch_id);
    if(itToInsert == patches.end() || *itToInsert != patch_id) {
        patches.insert(itToInsert, patch_id);
    }
}

bool PatchTreeAdditionValue::is_patch_id(int patch_id) const {
    std::vector<int>::const_iterator findIt = std::lower_bound(patches.begin(), patches.end(), patch_id);
    return findIt != patches.end() && *findIt == patch_id;
}

long PatchTreeAdditionValue::get_size() const {
    return patches.size();
}

std::string PatchTreeAdditionValue::to_string() const {
    std::string ret = "{";
    bool separator = false;
    for(int i = 0; i < patches.size(); i++) {
        if(separator) ret += ",";
        separator = true;
        ret += std::to_string(patches[i]);
    }
    ret += "}";
    return ret;
}

const char *PatchTreeAdditionValue::serialize(size_t *size) const {
    *size = patches.size() * sizeof(int);
    char* bytes = (char *) malloc(*size);
    for(int i = 0; i < patches.size(); i++) {
        std::memcpy(&bytes[i * sizeof(int)], &patches[i], sizeof(int));
    }
    return bytes;
}

void PatchTreeAdditionValue::deserialize(const char *data, size_t size) {
    size_t count = size / sizeof(int);
    patches.resize(count);
    for(int i = 0; i < count; i++) {
        std::memcpy(&patches.data()[i], &data[i * sizeof(int)], sizeof(int));
    }
}
