#include <cstring>
#include <string>
#include <iostream>

#include "patch_tree_deletion_value.h"

using namespace std;

int PatchTreeDeletionValueElement::get_patch_id() const {
    return patch_id;
}

const PatchPositions& PatchTreeDeletionValueElement::get_patch_positions() const {
    return patch_positions;
}

void PatchTreeDeletionValueElement::set_local_change() {
    local_change = true;
}

bool PatchTreeDeletionValueElement::is_local_change() const {
    return local_change;
}


PatchTreeDeletionValue::PatchTreeDeletionValue() {}

void PatchTreeDeletionValue::add(const PatchTreeDeletionValueElement& element) {
    std::vector<PatchTreeDeletionValueElement>::iterator itToInsert = std::lower_bound(
            elements.begin(), elements.end(), element);
    // Overwrite existing element if patch id already present, otherwise insert new element.
    if(itToInsert != elements.end() && itToInsert->get_patch_id() == element.get_patch_id()) {
        *itToInsert = element;
    } else {
        elements.insert(itToInsert, element);
    }
}

long PatchTreeDeletionValue::get_patchvalue_index(int patch_id) const {
    PatchTreeDeletionValueElement item(patch_id, PatchPositions());
    std::vector<PatchTreeDeletionValueElement>::const_iterator findIt = std::lower_bound(elements.begin(), elements.end(), item);
    if (findIt != elements.end() && findIt->get_patch_id() == patch_id) {
        return std::distance(elements.begin(), findIt);
    } else {
        return -1;
    }
}

long PatchTreeDeletionValue::get_size() const {
    return elements.size();
}

const PatchTreeDeletionValueElement& PatchTreeDeletionValue::get_patch(long element) const {
    return elements[element];
}

const PatchTreeDeletionValueElement& PatchTreeDeletionValue::get(int patch_id) const {
    long index = get_patchvalue_index(patch_id);
    if(index < 0 || index >= elements.size()) {
        return get_patch(elements.size() - 1);
        //throw std::invalid_argument("Index out of bounds (PatchTreeDeletionValue::get),"
        //                                    "tried to get patch id " + std::to_string(patch_id) + " in " +  this->to_string());
    }
    return get_patch(index);
}

bool PatchTreeDeletionValue::is_local_change(int patch_id) const {
    PatchTreeDeletionValueElement item(patch_id, PatchPositions());
    std::vector<PatchTreeDeletionValueElement>::const_iterator findIt = std::lower_bound(elements.begin(), elements.end(), item);
    if(findIt < elements.begin()) {
        throw std::runtime_error("Tried to retrieve a patch_id that was too low. (PatchTreeDeletionValue::is_local_change),"
                                         "tried to find " + std::to_string(patch_id) + " in " + this->to_string());
    }
    if(findIt >= elements.end()) findIt--;
    return findIt->is_local_change();
}

string PatchTreeDeletionValue::to_string() const {
    string ret = "{";
    bool separator = false;
    for(int i = 0; i < elements.size(); i++) {
        if(separator) ret += ",";
        separator = true;
        ret += std::to_string(elements[i].get_patch_id()) + ":" + elements[i].get_patch_positions().to_string();
    }
    ret += "}";
    return ret;
}

const char* PatchTreeDeletionValue::serialize(size_t* size) const {
    *size = elements.size() * sizeof(PatchTreeDeletionValueElement);
    char* bytes = (char *) malloc(*size);
    for(int i = 0; i < elements.size(); i++) {
        std::memcpy(&bytes[i * sizeof(PatchTreeDeletionValueElement)], &elements[i], sizeof(PatchTreeDeletionValueElement));
    }
    return bytes;
}

void PatchTreeDeletionValue::deserialize(const char* data, size_t size) {
    size_t count = size / sizeof(PatchTreeDeletionValueElement);
    elements.resize(count);
    for(int i = 0; i < count; i++) {
        std::memcpy(&elements.data()[i], &data[i * sizeof(PatchTreeDeletionValueElement)], sizeof(PatchTreeDeletionValueElement));
    }
}
