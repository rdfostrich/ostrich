#include <cstring>
#include <string>
#include <iostream>

#include "patch_tree_value.h"

using namespace std;

int PatchTreeValueElement::get_patch_id() const {
    return patch_id;
}

const PatchPositions& PatchTreeValueElement::get_patch_positions() const {
    return patch_positions;
}

bool PatchTreeValueElement::is_addition() const {
    return addition;
}

void PatchTreeValueElement::set_local_change() {
    local_change = true;
}

bool PatchTreeValueElement::is_local_change() const {
    return local_change;
}


PatchTreeValue::PatchTreeValue() {}

void PatchTreeValue::add(const PatchTreeValueElement& element) {
    std::vector<PatchTreeValueElement>::iterator itToInsert = std::lower_bound(
            elements.begin(), elements.end(), element);
    // Overwrite existing element if patch id already present, otherwise insert new element.
    if(itToInsert != elements.end() && itToInsert->get_patch_id() == element.get_patch_id()) {
        *itToInsert = element;
    } else {
        elements.insert(itToInsert, element);
    }
}

long PatchTreeValue::get_patchvalue_index(int patch_id) const {
    PatchTreeValueElement item(patch_id, PatchPositions(), -1);
    std::vector<PatchTreeValueElement>::const_iterator findIt = std::lower_bound(elements.begin(), elements.end(), item);
    if (findIt != elements.end() && findIt->get_patch_id() == patch_id) {
        return std::distance(elements.begin(), findIt);
    } else {
        return -1;
    }
}

long PatchTreeValue::get_size() const {
    return elements.size();
}

const PatchTreeValueElement& PatchTreeValue::get_patch(long element) const {
    return elements[element];
}

const PatchTreeValueElement& PatchTreeValue::get(int patch_id) const {
    long index = get_patchvalue_index(patch_id);
    if(index < 0 || index >= elements.size()) {
        return get_patch(elements.size() - 1);
        //throw std::invalid_argument("Index out of bounds (PatchTreeValue::get),"
        //                                    "tried to get patch id " + std::to_string(patch_id) + " in " +  this->to_string());
    }
    return get_patch(index);
}

bool PatchTreeValue::is_addition(int patch_id) const {
    PatchTreeValueElement item(patch_id, PatchPositions(), -1);
    std::vector<PatchTreeValueElement>::const_iterator findIt = std::lower_bound(elements.begin(), elements.end(), item);
    if(findIt < elements.begin()) {
        throw std::runtime_error("Tried to retrieve a patch_id that was too low. (PatchTreeValue::is_addition),"
                                         "tried to find " + std::to_string(patch_id) + " in " + this->to_string());
    }
    if(findIt >= elements.end()) findIt--;
    return findIt->is_addition();
}

bool PatchTreeValue::is_local_change(int patch_id) const {
    PatchTreeValueElement item(patch_id, PatchPositions(), -1);
    std::vector<PatchTreeValueElement>::const_iterator findIt = std::lower_bound(elements.begin(), elements.end(), item);
    if(findIt < elements.begin()) {
        throw std::runtime_error("Tried to retrieve a patch_id that was too low. (PatchTreeValue::is_local_change),"
                                         "tried to find " + std::to_string(patch_id) + " in " + this->to_string());
    }
    if(findIt >= elements.end()) findIt--;
    return findIt->is_local_change();
}

string PatchTreeValue::to_string() const {
    string ret = "{";
    bool separator = false;
    for(int i = 0; i < elements.size(); i++) {
        if(separator) ret += ",";
        separator = true;
        ret += std::to_string(elements[i].get_patch_id()) + ":" + elements[i].get_patch_positions().to_string()
              + "(" + (elements[i].is_addition() ? "+" : "-") + ")";
    }
    ret += "}";
    return ret;
}

const char* PatchTreeValue::serialize(size_t* size) const {
    *size = elements.size() * sizeof(PatchTreeValueElement);
    char* bytes = (char *) malloc(*size);
    for(int i = 0; i < elements.size(); i++) {
        std::memcpy(&bytes[i * sizeof(PatchTreeValueElement)], &elements[i], sizeof(PatchTreeValueElement));
    }
    return bytes;
}

void PatchTreeValue::deserialize(const char* data, size_t size) {
    size_t count = size / sizeof(PatchTreeValueElement);
    elements.resize(count);
    for(int i = 0; i < count; i++) {
        std::memcpy(&elements.data()[i], &data[i * sizeof(PatchTreeValueElement)], sizeof(PatchTreeValueElement));
    }
}
