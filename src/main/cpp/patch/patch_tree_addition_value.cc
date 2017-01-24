#include <cstring>
#include <string>
#include <stdlib.h>
#include <iostream>
#include "patch_tree_addition_value.h"

PatchTreeAdditionValue::PatchTreeAdditionValue() : patches(), local_changes() {}


void PatchTreeAdditionValue::add(int patch_id) {
    std::vector<int>::iterator itToInsert = std::lower_bound(
            patches.begin(), patches.end(), patch_id);
    if(itToInsert == patches.end() || *itToInsert != patch_id) {
        patches.insert(itToInsert, patch_id);
    }
}

bool PatchTreeAdditionValue::is_patch_id(int patch_id) const {
    return get_patchvalue_index(patch_id) >= 0;
}

long PatchTreeAdditionValue::get_patchvalue_index(int patch_id) const {
    std::vector<int>::const_iterator findIt = std::lower_bound(patches.begin(), patches.end(), patch_id);
    if (findIt != patches.end() && *findIt == patch_id) {
        return std::distance(patches.begin(), findIt);
    } else {
        return -1;
    }
}

int PatchTreeAdditionValue::get_patch_id_at(long i) const {
    return i < get_size() ? patches[i] : -1;
}

long PatchTreeAdditionValue::get_size() const {
    return patches.size();
}

void PatchTreeAdditionValue::set_local_change(int patch_id) {
    std::vector<int>::iterator itToInsert = std::lower_bound(
            local_changes.begin(), local_changes.end(), patch_id);
    if(itToInsert == local_changes.end() || *itToInsert != patch_id) {
        local_changes.insert(itToInsert, patch_id);
    }

}

bool PatchTreeAdditionValue::is_local_change(int patch_id) const {
    if (local_changes.size() == 0) return false;
    std::vector<int>::const_iterator findIt = std::lower_bound(local_changes.begin(), local_changes.end(), patch_id);
    if(findIt >= local_changes.end()) findIt--;
    return *findIt <= patch_id;
}

std::string PatchTreeAdditionValue::to_string() const {
    std::string ret = "{";
    bool separator = false;
    for(int i = 0; i < patches.size(); i++) {
        if(separator) ret += ",";
        separator = true;
        ret += std::to_string(patches[i]) + (is_local_change(patches[i]) ? "L" : "");
    }
    ret += "}";
    return ret;
}

const char *PatchTreeAdditionValue::serialize(size_t *size) const {
    size_t patches_size = patches.size();
    size_t local_changes_size = local_changes.size();
    size_t size_t_size_bits = sizeof(size_t);
    size_t patches_size_bits = patches_size * sizeof(int);
    size_t local_changes_size_bits = local_changes_size * sizeof(bool);

    *size = size_t_size_bits + patches_size_bits + local_changes_size_bits;
    char* bytes = (char *) malloc(*size);

    // Append patches count
    std::memcpy(bytes, &patches_size, size_t_size_bits);

    // Append patches
    for(int i = 0; i < patches.size(); i++) {
        std::memcpy(&bytes[size_t_size_bits + i * sizeof(int)], &patches[i], sizeof(int));
    }

    // Append local changes
    for(int i = 0; i < local_changes.size(); i++) {
        std::memcpy(&bytes[size_t_size_bits + patches_size_bits + i * sizeof(bool)], &local_changes[i], sizeof(bool));
    }
    return bytes;
}

void PatchTreeAdditionValue::deserialize(const char *data, size_t size) {
    size_t patches_size, local_changes_size;
    // Read patches count
    std::memcpy(&patches_size, data, sizeof(size_t));
    size_t size_t_size_bits = sizeof(size_t);
    // Calculate local changes size based on remaining bits
    local_changes_size = (size - size_t_size_bits - (patches_size * sizeof(int))) / sizeof(bool);
    size_t patches_size_bits = patches_size * sizeof(int);

    // Read patches
    patches.resize(patches_size);
    for(int i = 0; i < patches_size; i++) {
        std::memcpy(&patches.data()[i], &data[size_t_size_bits + i * sizeof(int)], sizeof(int));
    }

    // Read local changes
    local_changes.resize(local_changes_size);
    for(int i = 0; i < local_changes_size; i++) {
        std::memcpy(&local_changes.data()[i], &data[size_t_size_bits + patches_size_bits + i * sizeof(bool)], sizeof(bool));
    }
}
