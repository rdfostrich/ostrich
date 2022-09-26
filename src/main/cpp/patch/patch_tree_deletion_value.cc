#include <cstring>
#include <string>

#include "patch_tree_deletion_value.h"


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

#ifndef COMPRESSED_DEL_VALUES
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
    char* bytes = new char[*size];
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
#else
template <class T>
PatchTreeDeletionValueBase<T>::PatchTreeDeletionValueBase(int max_patch_id): max_patch_id(max_patch_id+1) {}

template <class T>
bool PatchTreeDeletionValueBase<T>::add(const T& element) {
    return elements.addition(element);
}

template <class T>
bool PatchTreeDeletionValueBase<T>::del(int patch_id) {
    return elements.deletion(patch_id);
}

template <class T>
long PatchTreeDeletionValueBase<T>::get_patchvalue_index(int patch_id) const {
    return elements.get_index(patch_id, max_patch_id);
}

template <class T>
long PatchTreeDeletionValueBase<T>::get_size() const {
    return elements.size(max_patch_id);
}

template <class T>
T PatchTreeDeletionValueBase<T>::get_patch(long element) const {
    return elements.get_element_at(element, max_patch_id);
}

template <class T>
T PatchTreeDeletionValueBase<T>::get(int patch_id) const {
    long index = get_patchvalue_index(patch_id);
    long size = elements.size(max_patch_id);
    if(index < 0 || index >= size) {
        return get_patch(size - 1);
    }
    return get_patch(index);
}

template <class T>
bool PatchTreeDeletionValueBase<T>::is_local_change(int patch_id) const {
    if (get_size() == 0) return false;
    long index = get_patchvalue_index(patch_id);
    if (index >= 0) {
        return get_patch(index).is_local_change();
    }
    return get_patch(get_size()-1).is_local_change();
}

template <class T>
string PatchTreeDeletionValueBase<T>::to_string() const {
    string ret = "{";
    bool separator = false;
    for(int i = 0; i < elements.size(max_patch_id); i++) {
        if(separator) ret += ",";
        separator = true;
        ret += elements.get_element_at(i, max_patch_id).to_string();
    }
    ret += "}";
    return ret;
}

template <class T>
const char* PatchTreeDeletionValueBase<T>::serialize(size_t* size) const {
    auto data = elements.serialize();
    *size = data.second;
    return data.first;
}

template <class T>
void PatchTreeDeletionValueBase<T>::deserialize(const char* data, size_t size) {
    elements.deserialize(data, size);
}
#endif


bool IntervalPatchPositionsContainer::insert_positions(int patch_id, const PatchPositions &positions) {
    bool has_changed = false;
    auto pos = positions_map.lower_bound(patch_id);
    if (pos == positions_map.end()) {
        if (pos != positions_map.begin()) {
            pos--;
            if (pos->first < patch_id && pos->second.first == patch_id) {
                if (pos->second.second == positions) {
                    positions_map[pos->first] = std::make_pair(max, positions);
                } else {
                    positions_map[patch_id] = std::make_pair(max, positions);
                }
                has_changed = true;
            } else if (pos->second.first < patch_id) {
                positions_map[patch_id] = std::make_pair(max, positions);
                has_changed = true;
            } else if (pos->second.second != positions) {
                pos->second.first = patch_id;
                positions_map[patch_id] = std::make_pair(max, positions);
                has_changed = true;
            }
        } else {
            positions_map.insert(std::make_pair(patch_id, std::make_pair(max, positions)));
            has_changed = true;
        }
    } else {
        if (patch_id < pos->first) {
            pos--;
            if (pos->second.first < patch_id) {
                pos++;
                if (pos->second.second == positions) {
                    std::pair<int, PatchPositions> old_del_val = pos->second;
                    positions_map.erase(pos);
                    positions_map.insert(std::make_pair(patch_id, old_del_val));
                } else {
                    positions_map[patch_id] = std::make_pair(pos->first, positions);
                }
                has_changed = true;
            } else if (pos->second.first == patch_id) {
                auto tmp_pos = pos;
                tmp_pos++;
                if (pos->second.second == positions) {
                    if (tmp_pos->second.second == positions) {
                        if (tmp_pos != positions_map.end()) {
                            std::pair<int, PatchPositions> old_del_val = tmp_pos->second;
                            positions_map.erase(tmp_pos);
                            pos->second = old_del_val;
                        }
                    } else {
                        int next_change_id = tmp_pos->first;
                        pos->second = std::make_pair(next_change_id, positions);
                    }
                } else {
                    int next_change_id = tmp_pos->first;
                    positions_map[patch_id] = std::make_pair(next_change_id, positions);
                }
                has_changed = true;
            }
        } else if (pos->second.second != positions) {
            pos->second.second = positions; // overwrite
            has_changed = true;
        }
    }
    return has_changed;
}

bool IntervalPatchPositionsContainer::delete_positions(int patch_id) {
    if (positions_map.empty()) {
        return false;
    }
    auto pos = positions_map.lower_bound(patch_id);
    if (pos == positions_map.end() || patch_id < pos->first) {
        pos--;
    }
    if (patch_id < pos->second.first) {
        if (pos->first == patch_id) {
            positions_map.erase(pos);
        } else {
            pos->second.first = patch_id;
        }
        return true;
    }
    return false;
}

bool IntervalPatchPositionsContainer::get_positions(int patch_id, PatchPositions &positions, int outer_patch_limit) const {
    for (auto inter: positions_map) {
        int max_value = inter.second.first == max ? outer_patch_limit : inter.second.first;
        if (inter.first <= patch_id && patch_id < max_value) {
            positions = inter.second.second;
            return true;
        }
    }
    return false;
}

std::pair<const char *, size_t> IntervalPatchPositionsContainer::serialize() const {
    size_t size = positions_map.size() * (2*sizeof(int)+sizeof(PatchPositions));
    char* data = new char[size];
    size_t offset = 0;
    for (const auto& inter_p: positions_map) {
        std::memcpy(data+offset, &inter_p.first, sizeof(int));
        offset += sizeof(int);
        std::memcpy(data+offset, &inter_p.second.first, sizeof(int));
        offset += sizeof(int);
        std::memcpy(data+offset, &inter_p.second.second, sizeof(PatchPositions));
        offset += sizeof(PatchPositions);
    }
    return std::make_pair(data, size);
}

void IntervalPatchPositionsContainer::deserialize(const char *data, size_t size) {
    positions_map.clear();
    auto it = positions_map.begin();
    for (size_t offset=0; offset<size; offset+=(2*sizeof(int) + sizeof(PatchPositions))) {
        int s, e;
        std::memcpy(&s, data+offset, sizeof(int));
        std::memcpy(&e, data+offset+sizeof(int), sizeof(int));
        PatchPositions p;
        std::memcpy(&p, data+offset+2*sizeof(int), sizeof(PatchPositions));
        positions_map.emplace_hint(it, s, std::make_pair(e,p));
        it = positions_map.end();
    }
}


void
DeltaPatchPositionsContainerBase::delta_serialize_position_vec(const vector<VerPatchPositions<int>> &position_vec, char **data,
                                                               size_t *size) {
    auto position_diff = [](const VerPatchPositions<int>& first, const VerPatchPositions<int>& second) {
        uint8_t head = 0;
        std::vector<PatchPosition> diff_values;

        PatchPosition diff = second.positions.sp_ - first.positions.sp_;
        head |= (bool)diff;
        if (diff) diff_values.push_back(diff);

        diff = second.positions.s_o - first.positions.s_o;
        head |= (bool)diff << 1;
        if (diff) diff_values.push_back(diff);

        diff = second.positions.s__ - first.positions.s__;
        head |= (bool)diff << 2;
        if (diff) diff_values.push_back(diff);

        diff = second.positions._po - first.positions._po;
        head |= (bool)diff << 3;
        if (diff) diff_values.push_back(diff);

        diff = second.positions._p_ - first.positions._p_;
        head |= (bool)diff << 4;
        if (diff) diff_values.push_back(diff);

        diff = second.positions.__o - first.positions.__o;
        head |= (bool)diff << 5;
        if (diff) diff_values.push_back(diff);

        diff = second.positions.___ - first.positions.___;
        head |= (bool)diff << 6;
        if (diff) diff_values.push_back(diff);

        size_t size = sizeof(uint8_t) + sizeof(int) + diff_values.size()*sizeof(PatchPosition);
        char* data = new char[size];
        std::memcpy(data, &head, sizeof(uint8_t));
        std::memcpy(data+sizeof(uint8_t), &second.patch_id, sizeof(int));
        std::memcpy(data+sizeof(uint8_t)+sizeof(int), diff_values.data(), diff_values.size()*sizeof(PatchPosition));

        return std::make_pair(size, data);
    };

    *size = 0;
    if (position_vec.size() == 1) {
        *size = sizeof(VerPatchPositions<int>);
        *data = new char[*size];
        std::memcpy(*data, &position_vec[0].patch_id, sizeof(int));
        std::memcpy(*data+sizeof(int), &position_vec[0].positions, sizeof(PatchPositions));
    } else if (position_vec.size() > 1) { // if more than 1 PatchPositions, we use delta-encoding
        // We can't know before encoding the total size. PatchPositions + (>= 1 bytes)
        size_t total_size = sizeof(VerPatchPositions<int>);
        std::vector<std::pair<size_t, char*>> data_vec;
        data_vec.reserve(position_vec.size());
        for (size_t i=1; i<position_vec.size(); i++) {  // Loop to perform delta encoding (i-1 & ith elements)
            auto d = position_diff(position_vec[i-1], position_vec[i]);
            total_size += d.first;
            data_vec.push_back(d);
        }
        // We allocate the correct amount of memory
        *size = total_size;
        *data = new char[total_size];
        std::memcpy(*data, &position_vec[0].patch_id, sizeof(int));
        std::memcpy(*data+sizeof(int), &position_vec[0].positions, sizeof(PatchPositions)); // plain copy of 1st PatchPositions
        size_t offset = sizeof(VerPatchPositions<int>);
        for (auto& p: data_vec) {  // loop to copy the "delta" PatchPositions
            std::memcpy(*data+offset, p.second, p.first);
            offset += p.first;
            delete[] p.second;
        }
    }
}

void
DeltaPatchPositionsContainerBase::delta_deserialize_position_vec(vector<VerPatchPositions<int>> &position_vec, const char *data,
                                                                 size_t size) {
    auto position_undiff = [] (const char* data, const PatchPositions& prev, VerPatchPositions<int>& pos) {
        uint8_t head;
        size_t offset = 0;
        std::memcpy(&head, data+offset, sizeof(uint8_t));
        offset += sizeof(uint8_t);
        std::memcpy(&pos.patch_id, data+offset, sizeof(int));
        offset += sizeof(int);

        PatchPosition diff = 0;
        if (head & 1) {
            std::memcpy(&diff, data+offset, sizeof(PatchPosition));
            offset += sizeof(PatchPosition);
        }
        pos.positions.sp_ = prev.sp_ + diff;

        diff = 0;
        head >>= 1;
        if (head & 1) {
            std::memcpy(&diff, data+offset, sizeof(PatchPosition));
            offset += sizeof(PatchPosition);
        }
        pos.positions.s_o = prev.s_o + diff;

        diff = 0;
        head >>= 1;
        if (head & 1) {
            std::memcpy(&diff, data+offset, sizeof(PatchPosition));
            offset += sizeof(PatchPosition);
        }
        pos.positions.s__ = prev.s__ + diff;

        diff = 0;
        head >>= 1;
        if (head & 1) {
            std::memcpy(&diff, data+offset, sizeof(PatchPosition));
            offset += sizeof(PatchPosition);
        }
        pos.positions._po = prev._po + diff;

        diff = 0;
        head >>= 1;
        if (head & 1) {
            std::memcpy(&diff, data+offset, sizeof(PatchPosition));
            offset += sizeof(PatchPosition);
        }
        pos.positions._p_ = prev._p_ + diff;

        diff = 0;
        head >>= 1;
        if (head & 1) {
            std::memcpy(&diff, data+offset, sizeof(PatchPosition));
            offset += sizeof(PatchPosition);
        }
        pos.positions.__o = prev.__o + diff;

        diff = 0;
        head >>= 1;
        if (head & 1) {
            std::memcpy(&diff, data+offset, sizeof(PatchPosition));
            offset += sizeof(PatchPosition);
        }
        pos.positions.___ = prev.___ + diff;

        return offset;
    };

    position_vec.resize(1);
    std::memcpy(&position_vec[0].patch_id, data, sizeof(int));
    std::memcpy(&position_vec[0].positions, data+sizeof(int), sizeof(PatchPositions));
    size_t offset = sizeof(VerPatchPositions<int>);
    size_t prev_index = 0;
    while(offset < size) {
        position_vec.emplace_back();  // insert default value at the back the vector to be modified
        offset += position_undiff(data+offset, position_vec[prev_index].positions, position_vec.back());
        prev_index += 1;
    }
}

void DeltaPatchPositionsContainerBase::serialize_position_vec(const vector<VerPatchPositions<int>> &position_vec,
                                                              char **data, size_t *size) {
    *size = position_vec.size() * sizeof(VerPatchPositions<int>);
    if (*size > 0) {
        *data = new char[*size];
        size_t offset = 0;
        for (auto& vp: position_vec) {
//            std::memcpy(*data+offset, &vp.patch_id, sizeof(VerPatchPositions<int>));
//            std::memcpy(*data+offset+sizeof(int), &vp.patch_id, sizeof(VerPatchPositions<int>));
            std::memcpy(*data+offset, &vp, sizeof(VerPatchPositions<int>));
            offset += sizeof(VerPatchPositions<int>);
        }
    }
}

void DeltaPatchPositionsContainerBase::deserialize_position_vec(vector<VerPatchPositions<int>> &position_vec,
                                                                const char *data, size_t size) {
    size_t n_elements = size/sizeof(VerPatchPositions<int>);
    position_vec.resize(n_elements);
    for (size_t i=0; i<n_elements; i++) {
        size_t offset = i*sizeof(VerPatchPositions<int>);
        std::memcpy(&position_vec[i], data+offset, sizeof(VerPatchPositions<int>));
    }
}


bool DeltaPatchPositionsContainer::insert_positions(int patch_id, const PatchPositions &positions) {
    VerPatchPositions<int> search_item = {patch_id, positions};
    auto pos = std::lower_bound(position_vec.begin(), position_vec.end(), search_item);
    // Overwrite existing element if patch id already present, otherwise insert new element.
    if(pos != position_vec.end() && pos->patch_id == patch_id) {
        *pos = search_item;
    } else {
        position_vec.insert(pos, search_item);
    }
    return true;
}

bool DeltaPatchPositionsContainer::delete_positions(int patch_id) {
    VerPatchPositions<int> search_item = {patch_id, PatchPositions()};
    auto pos = std::lower_bound(position_vec.begin(), position_vec.end(), search_item);
    if (pos != position_vec.end() && pos->patch_id == patch_id) {
        position_vec.erase(pos);
        return true;
    }
    return false;
}

bool DeltaPatchPositionsContainer::get_positions(int patch_id, PatchPositions &positions, int outer_patch_limit) const {
    VerPatchPositions<int> search_item = {patch_id, PatchPositions()};
    auto pos = std::lower_bound(position_vec.begin(), position_vec.end(), search_item);
    if (pos != position_vec.end() && pos->patch_id == patch_id) {
        positions = pos->positions;
        return true;
    }
    return false;
}

std::pair<const char *, size_t> DeltaPatchPositionsContainer::serialize() const {
    char* data = nullptr;
    size_t size;
    DeltaPatchPositionsContainerBase::delta_serialize_position_vec(position_vec, &data, &size);
//    DeltaPatchPositionsContainerBase::serialize_position_vec(position_vec, &data, &size);
    return std::make_pair(data, size);
}

void DeltaPatchPositionsContainer::deserialize(const char *data, size_t size) {
    DeltaPatchPositionsContainerBase::delta_deserialize_position_vec(position_vec, data, size);
//    DeltaPatchPositionsContainerBase::deserialize_position_vec(position_vec, data, size);
}


bool DeltaPatchPositionsContainerV2::insert_positions(int patch_id, const PatchPositions &positions) {
    bool has_changed = false;
    VerPatchPositions<int> verpos = {patch_id, positions};
    if (position_vec.empty()) {
        position_vec.push_back(verpos);
        has_changed = true;
    } else {
        auto it_insert = std::lower_bound(position_vec.begin(), position_vec.end(), verpos);
        if (it_insert == position_vec.end() || *it_insert > verpos) it_insert--;
        if ((*it_insert).patch_id == verpos.patch_id) {  // patch_id equal
            *it_insert = verpos;  // we overwrite
            has_changed = true;
        } else if ((*it_insert).positions != verpos.positions) {  // it_insert < verpos -> we have new PatchPositions, so we insert
            it_insert = position_vec.insert(++it_insert, verpos);
            has_changed = true;
        } // else PatchPositions are equal we don't need to do anything
    }
    return has_changed;
}

bool DeltaPatchPositionsContainerV2::delete_positions(int patch_id) {
    return false;
}

bool
DeltaPatchPositionsContainerV2::get_positions(int patch_id, PatchPositions &positions, int outer_patch_limit) const {
    if (position_vec.empty()) {
        return false;
    }
    VerPatchPositions<int> search_item = {patch_id, PatchPositions()};  // temporary for search
    auto pos = std::lower_bound(position_vec.begin(), position_vec.end(), search_item);
    if (pos == position_vec.end() || pos->patch_id > search_item.patch_id) pos--;
    if (pos->patch_id <= search_item.patch_id) {
        positions = pos->positions;
        return true;
    }
    return false;
}

std::pair<const char *, size_t> DeltaPatchPositionsContainerV2::serialize() const {
    char* data = nullptr;
    size_t size;
    DeltaPatchPositionsContainerBase::delta_serialize_position_vec(position_vec, &data, &size);
//    DeltaPatchPositionsContainerBase::serialize_position_vec(position_vec, &data, &size);
    return std::make_pair(data, size);
}

void DeltaPatchPositionsContainerV2::deserialize(const char *data, size_t size) {
    DeltaPatchPositionsContainerBase::delta_deserialize_position_vec(position_vec, data, size);
//    DeltaPatchPositionsContainerBase::deserialize_position_vec(position_vec, data, size);
}


template class PatchTreeDeletionValueBase<PatchTreeDeletionValueElement>;
template class PatchTreeDeletionValueBase<PatchTreeDeletionValueElementBase>;
