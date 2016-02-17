#include <string>
#include <iostream>

#include "patch_tree_value.h"

using namespace std;

int PatchTreeValueElement::get_patch_id() {
    return patch_id;
}

PatchPositions PatchTreeValueElement::get_patch_positions() {
    return patch_positions;
}

bool PatchTreeValueElement::is_addition() {
    return addition;
}

PatchTreeValue::PatchTreeValue() {}

void PatchTreeValue::add(PatchTreeValueElement element) {
    std::vector<PatchTreeValueElement>::iterator itToInsert = std::lower_bound(
            elements.begin(), elements.end(), element);
    elements.insert(itToInsert, element);
}

long PatchTreeValue::get_patchvalue_index(int patch_id) {
    PatchTreeValueElement item(patch_id, PatchPositions(), -1);
    std::vector<PatchTreeValueElement>::iterator findIt = std::lower_bound(elements.begin(), elements.end(), item);
    if (findIt != elements.end() && findIt->get_patch_id() == patch_id) {
        return std::distance(elements.begin(), findIt);
    } else {
        return -1;
    }
}

PatchTreeValueElement PatchTreeValue::get_patch(long element) {
    return elements[element];
}

PatchTreeValueElement PatchTreeValue::get(int patch_id) {
    long index = get_patchvalue_index(patch_id);
    if(index < 0 || index >= elements.size()) {
        throw std::invalid_argument("Index out of bounds");
    }
    return get_patch(index);
}

string PatchTreeValue::to_string() {
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

const char* PatchTreeValue::serialize(size_t* size) {
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
