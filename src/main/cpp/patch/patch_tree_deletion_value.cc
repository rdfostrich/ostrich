#include <cstring>
#include <string>
#include <iostream>

#include "patch_tree_deletion_value.h"

using namespace std;

int PatchTreeDeletionValueElementBase::get_patch_id() const {
    return patch_id;
}

const PatchPositions& PatchTreeDeletionValueElement::get_patch_positions() const {
    return patch_positions;
}

string PatchTreeDeletionValueElement::to_string() const {
    return PatchTreeDeletionValueElementBase::to_string() + ":" + get_patch_positions().to_string();
}

void PatchTreeDeletionValueElementBase::set_local_change() {
    local_change = true; // TODO: Can we instead just set all patch positions to -1 to save some storage space?
}

bool PatchTreeDeletionValueElementBase::is_local_change() const {
    return local_change;
}

string PatchTreeDeletionValueElementBase::to_string() const {
    return std::to_string(get_patch_id());
}

template <class T>
PatchTreeDeletionValueBase<T>::PatchTreeDeletionValueBase() {}

template <class T>
void PatchTreeDeletionValueBase<T>::add(const T& element) {
    typename std::vector<T>::iterator itToInsert = std::lower_bound(
            elements.begin(), elements.end(), element);
    // Overwrite existing element if patch id already present, otherwise insert new element.
    if(itToInsert != elements.end() && itToInsert->get_patch_id() == element.get_patch_id()) {
        *itToInsert = element;
    } else {
        elements.insert(itToInsert, element);
    }
}

template <class T>
long PatchTreeDeletionValueBase<T>::get_patchvalue_index(int patch_id) const {
    T item(patch_id);
    typename std::vector<T>::const_iterator findIt = std::lower_bound(elements.begin(), elements.end(), item);
    if (findIt != elements.end() && findIt->get_patch_id() == patch_id) {
        return std::distance(elements.begin(), findIt);
    } else {
        return -1;
    }
}

template <class T>
long PatchTreeDeletionValueBase<T>::get_size() const {
    return elements.size();
}

template <class T>
const T& PatchTreeDeletionValueBase<T>::get_patch(long element) const {
    return elements[element];
}

template <class T>
const T& PatchTreeDeletionValueBase<T>::get(int patch_id) const {
    long index = get_patchvalue_index(patch_id);
    if(index < 0 || index >= elements.size()) {
        return get_patch(elements.size() - 1);
        //throw std::invalid_argument("Index out of bounds (PatchTreeDeletionValue::get),"
        //                                    "tried to get patch id " + std::to_string(patch_id) + " in " +  this->to_string());
    }
    return get_patch(index);
}

template <class T>
bool PatchTreeDeletionValueBase<T>::is_local_change(int patch_id) const {
    if (get_size() == 0) return false;
    T item(patch_id);
    typename std::vector<T>::const_iterator findIt = std::lower_bound(elements.begin(), elements.end(), item);
    if(findIt < elements.begin()) {
        throw std::runtime_error("Tried to retrieve a patch_id that was too low. (PatchTreeDeletionValue::is_local_change),"
                                         "tried to find " + std::to_string(patch_id) + " in " + this->to_string());
    }
    if(findIt >= elements.end()) findIt--;
    return findIt->is_local_change();
}

template <class T>
string PatchTreeDeletionValueBase<T>::to_string() const {
    string ret = "{";
    bool separator = false;
    for(int i = 0; i < elements.size(); i++) {
        if(separator) ret += ",";
        separator = true;
        ret += elements[i].to_string();
    }
    ret += "}";
    return ret;
}

template <class T>
const char* PatchTreeDeletionValueBase<T>::serialize(size_t* size) const {
    *size = elements.size() * sizeof(T);
    char* bytes = (char *) malloc(*size);
    for(int i = 0; i < elements.size(); i++) {
        std::memcpy(&bytes[i * sizeof(T)], &elements[i], sizeof(T));
    }
    return bytes;
}

template <class T>
void PatchTreeDeletionValueBase<T>::deserialize(const char* data, size_t size) {
    size_t count = size / sizeof(T);
    elements.resize(count);
    for(int i = 0; i < count; i++) {
        std::memcpy(&elements.data()[i], &data[i * sizeof(T)], sizeof(T));
    }
}

template class PatchTreeDeletionValueBase<PatchTreeDeletionValueElement>;
template class PatchTreeDeletionValueBase<PatchTreeDeletionValueElementBase>;
