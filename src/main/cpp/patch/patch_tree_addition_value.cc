#include <cstring>
#include <string>
#include <cstdlib>
#include <limits>
#include "patch_tree_addition_value.h"


#ifndef COMPRESSED_TREE_VALUES
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
    size_t local_changes_size_bits = local_changes_size * sizeof(int);

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
        std::memcpy(&bytes[size_t_size_bits + patches_size_bits + i * sizeof(int)], &local_changes[i], sizeof(int));
    }
    return bytes;
}

void PatchTreeAdditionValue::deserialize(const char *data, size_t size) {
    size_t patches_size, local_changes_size;
    // Read patches count
    std::memcpy(&patches_size, data, sizeof(size_t));
    size_t size_t_size_bits = sizeof(size_t);
    // Calculate local changes size based on remaining bits
    local_changes_size = (size - size_t_size_bits - (patches_size * sizeof(int))) / sizeof(int);
    size_t patches_size_bits = patches_size * sizeof(int);

    // Read patches
    patches.resize(patches_size);
    for(int i = 0; i < patches_size; i++) {
        std::memcpy(&patches[i], &data[size_t_size_bits + i * sizeof(int)], sizeof(int));
    }

    // Read local changes
    local_changes.resize(local_changes_size);
    for(int i = 0; i < local_changes_size; i++) {
        std::memcpy(&local_changes[i], &data[size_t_size_bits + patches_size_bits + i * sizeof(int)], sizeof(int));
    }
}

#else

PatchTreeAdditionValue::PatchTreeAdditionValue(int max_patch_id) : patches(std::numeric_limits<int>::max()),
                                                   local_changes(std::numeric_limits<int>::max()),
                                                   max_patch_id(max_patch_id) {}

bool PatchTreeAdditionValue::add(int patch_id) {
    return patches.addition(patch_id);
}

bool PatchTreeAdditionValue::del(int patch_id) {
    return patches.deletion(patch_id);
}

bool PatchTreeAdditionValue::is_patch_id(int patch_id) const {
    return patches.is_in(patch_id);
}

int PatchTreeAdditionValue::get_patch_id_at(long i) const {
    int index = 0;
    for (auto inter: patches.get_internal_representation()) {
        int max_value = inter.second == patches.get_max_value() ? max_patch_id : inter.second;
        int range = max_value - inter.first;
        if (i < index + range) {
            return inter.first + (i - index);
        }
        index += range;
    }
    return -1;
}

long PatchTreeAdditionValue::get_size() const {
    int size = 0;
    for (auto inter: patches.get_internal_representation()) {
        int max_value = inter.second == patches.get_max_value() ? max_patch_id : inter.second;
        int range = max_value - inter.first;
        size += range;
    }
    return size;
}

bool PatchTreeAdditionValue::set_local_change(int patch_id) {
    return local_changes.addition(patch_id);
}

bool PatchTreeAdditionValue::unset_local_change(int patch_id) {
    return local_changes.deletion(patch_id);
}

bool PatchTreeAdditionValue::is_local_change(int patch_id) const {
    return local_changes.is_in(patch_id);
}

std::string PatchTreeAdditionValue::to_string() const {
    std::string ret = "{";
    bool separator = false;
    for(int i = 0; i < get_size(); i++) {
        if(separator) ret += ",";
        separator = true;
        int p = get_patch_id_at(i);
        ret += std::to_string(p) + (is_local_change(p) ? "L" : "");
    }
    ret += "}";
    return ret;
}

const char *PatchTreeAdditionValue::serialize(size_t *size) const {
    auto bin_patches = patches.serialize();
    auto bin_local = local_changes.serialize();
    *size = bin_patches.second + bin_local.second + sizeof(size_t);
    char* data = new char[*size];
    std::memcpy(data, &bin_patches.second, sizeof(size_t));
    std::memcpy(data+sizeof(size_t), bin_patches.first, bin_patches.second);
    if (bin_local.second > 0) {
        std::memcpy(data+sizeof(size_t)+bin_patches.second, bin_local.first, bin_local.second);
    }
    delete[] bin_patches.first;
    delete[] bin_local.first;
    return data;
}

void PatchTreeAdditionValue::deserialize(const char *data, size_t size) {
    size_t patches_size;
    std::memcpy(&patches_size, data, sizeof(size_t));
    size_t local_change_size = size - patches_size;
    patches.deserialize(data+sizeof(size_t), patches_size);
    if (local_change_size > 0) {
        local_changes.deserialize(data+sizeof(size_t)+patches_size, local_change_size);
    }
    delete[] data;
}


#endif


