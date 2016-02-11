#include <string>
#include <iostream>

#include "patch_tree_value.h"

using namespace std;

PatchTreeValue::PatchTreeValue() {}

void PatchTreeValue::add(PatchTreeValueElement element) {
    elements.push_back(element);
}

bool PatchTreeValue::contains(int patch_id) {
    for(int i = 0; i < elements.size(); i++) {
        if(elements[i].patch_id == patch_id) {
            return true;
        }
    }
    return false;
}

PatchTreeValueElement PatchTreeValue::get(int patch_id) {
    // This can alternatively be implemented as a map for improving lookup efficiency
    // But we have to make sure that serialization is fast
    for(int i = 0; i < elements.size(); i++) {
        if(elements[i].patch_id == patch_id) {
            return elements[i];
        }
    }
    throw std::invalid_argument("Patch id not present.");
}

string PatchTreeValue::to_string() {
    string ret = "{";
    bool separator = false;
    for(int i = 0; i < elements.size(); i++) {
        if(separator) ret += ",";
        separator = true;
        ret += std::to_string(elements[i].patch_id) + ":" + std::to_string(elements[i].patch_position)
              + "(" + (elements[i].addition ? "+" : "-") + ")";
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
