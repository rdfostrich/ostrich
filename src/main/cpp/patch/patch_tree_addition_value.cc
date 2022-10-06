#include <cstring>
#include <string>
#include <limits>
#include "patch_tree_addition_value.h"


#ifndef COMPRESSED_ADD_VALUES
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
    if(findIt == local_changes.end()) findIt--;
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
#ifdef USE_VSI
    size_t size_t_size_bytes = get_ULEB128_size(std::numeric_limits<size_t>::max());
    size_t int_size_bytes = get_SLEB128_size(std::numeric_limits<int>::max());
    size_t patches_size_bytes = patches_size * int_size_bytes;
    size_t local_changes_size_bytes = local_changes_size * int_size_bytes;
#else
    size_t size_t_size_bytes = sizeof(size_t);
    size_t patches_size_bytes = patches_size * sizeof(int);
    size_t local_changes_size_bytes = local_changes_size * sizeof(int);
#endif
    size_t alloc_size = size_t_size_bytes + patches_size_bytes + local_changes_size_bytes;
    char* bytes = new char[alloc_size];

#ifdef USE_VSI
    // Encode and append patches count
    std::vector<uint8_t> buffer;
    encode_ULEB128(patches_size, buffer);
    std::memcpy(bytes, buffer.data(), buffer.size());
    *size = buffer.size();
    buffer.clear();

    // Encode and append patches
    for (auto p: patches) {
        encode_SLEB128(p, buffer);
        std::memcpy(bytes+*size, buffer.data(), buffer.size());
        *size += buffer.size();
        buffer.clear();
    }

    // Encode and append local changes
    for (auto l: local_changes) {
        encode_SLEB128(l, buffer);
        std::memcpy(bytes+*size, buffer.data(), buffer.size());
        *size += buffer.size();
        buffer.clear();
    }
#else
    // Append patches count
    std::memcpy(bytes, &patches_size, size_t_size_bytes);

    // Append patches
    for(int i = 0; i < patches.size(); i++) {
        std::memcpy(&bytes[size_t_size_bytes + i * sizeof(int)], &patches[i], sizeof(int));
    }

    // Append local changes
    for(int i = 0; i < local_changes.size(); i++) {
        std::memcpy(&bytes[size_t_size_bytes + patches_size_bytes + i * sizeof(int)], &local_changes[i], sizeof(int));
    }
    *size = alloc_size;
#endif
    return bytes;
}

void PatchTreeAdditionValue::deserialize(const char *data, size_t size) {
#ifdef USE_VSI
    size_t patches_size, decode_size, offset;
    patches_size = decode_ULEB128((const uint8_t*)data, &decode_size);
    offset = decode_size;

    // Read patches
    patches.resize(patches_size);
    for (int i=0; i<patches_size; i++) {
        patches[i] = decode_SLEB128((const uint8_t*)(data+offset), &decode_size);
        offset += decode_size;
    }

    // Read local changes (the remaining of the data)
    local_changes.clear();
    while (offset < size) {
        local_changes.push_back(decode_SLEB128((const uint8_t*)(data+offset), &decode_size));
        offset += decode_size;
    }
#else
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
#endif
}

#else

PatchTreeAdditionValue::PatchTreeAdditionValue(int max_patch_id) : patches(std::numeric_limits<int>::max()),
                                                   local_changes(std::numeric_limits<int>::max()),
                                                   max_patch_id(max_patch_id+1) {}

bool PatchTreeAdditionValue::add(int patch_id) {
    if (patch_id >= max_patch_id) {
        max_patch_id = patch_id+1;
    }
    return patches.addition(patch_id);
}

bool PatchTreeAdditionValue::add_unique(int patch_id) {
    if (patch_id >= max_patch_id) {
        max_patch_id = patch_id+1;
    }
    bool hs = patches.lone_addition(patch_id);
    return hs;
}

bool PatchTreeAdditionValue::del(int patch_id) {
    if (patch_id >= max_patch_id) {
        max_patch_id = patch_id+1;
    }
    return patches.deletion(patch_id);
}

bool PatchTreeAdditionValue::is_patch_id(int patch_id) const {
    if (patch_id >= 0 && patch_id < max_patch_id) {
        return patches.is_in(patch_id);
    }
    return false;
}

long PatchTreeAdditionValue::get_patchvalue_index(int patch_id) const {
    return patches.get_index(patch_id, max_patch_id);
}

int PatchTreeAdditionValue::get_patch_id_at(long i) const {
    int p = patches.get_element_at(i, max_patch_id);
    return p == patches.get_max_value() ? -1 : p;
}

long PatchTreeAdditionValue::get_size() const {
    return patches.get_size(max_patch_id);
}

bool PatchTreeAdditionValue::set_local_change(int patch_id) {
    return local_changes.addition(patch_id);
}

bool PatchTreeAdditionValue::set_local_change_unique(int patch_id) {
    return local_changes.lone_addition(patch_id);
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
#ifdef USE_VSI
    size_t size_t_size_bytes = get_ULEB128_size(std::numeric_limits<size_t>::max());
#else
    size_t size_t_size_bytes = sizeof(size_t);
#endif
    size_t alloc_size = bin_patches.second + bin_local.second + size_t_size_bytes;
    char* data = new char[alloc_size];
#ifdef USE_VSI
    std::vector<uint8_t> buffer;
    encode_ULEB128(bin_patches.second, buffer);
    std::memcpy(data, buffer.data(), buffer.size());
    *size = buffer.size();
#else
    std::memcpy(data, &bin_patches.second, sizeof(size_t));
    *size = sizeof(size_t);
#endif
    std::memcpy(data+*size, bin_patches.first, bin_patches.second);
    *size += bin_patches.second;
    if (bin_local.second > 0) {
        std::memcpy(data+*size, bin_local.first, bin_local.second);
        *size += bin_local.second;
    }
    delete[] bin_patches.first;
    delete[] bin_local.first;
    return data;
}

void PatchTreeAdditionValue::deserialize(const char *data, size_t size) {
    size_t patches_size, decode_size, offset;
#ifdef USE_VSI
    patches_size = decode_ULEB128((const uint8_t*)data, &decode_size);
    offset = decode_size;
#else
    std::memcpy(&patches_size, data, sizeof(size_t));
    offset = sizeof(size_t);
#endif
    size_t local_change_size = size - patches_size - offset;
    patches.deserialize(data+offset, patches_size);
    offset += patches_size;
    if (local_change_size > 0) {
        local_changes.deserialize(data+offset, local_change_size);
    } else {
        local_changes.clear();
    }
}

#endif


