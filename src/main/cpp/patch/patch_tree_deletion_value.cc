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

char *PatchTreeDeletionValueElement::serialize(size_t *size) const {
    char* base_data = PatchTreeDeletionValueElementBase::serialize(size);
    char* data = new char[*size + PatchPositions::max_serialization_size()];
    std::memcpy(data, base_data, *size);
    delete[] base_data;

    size_t pos_size;
    char* pos_data = patch_positions.serialize(&pos_size);
    std::memcpy(data+*size, pos_data, pos_size);
    *size += pos_size;
    delete[] pos_data;

    return data;
}

size_t PatchTreeDeletionValueElement::deserialize(const char *data) {
    size_t consumed_size = PatchTreeDeletionValueElementBase::deserialize(data);
    consumed_size += patch_positions.deserialize(data+consumed_size);
    return consumed_size;
}

void PatchTreeDeletionValueElementBase::set_local_change() {
    local_change = true;  // TODO: Can we instead just set all patch positions to -1 to save some storage space?
}

bool PatchTreeDeletionValueElementBase::is_local_change() const {
    return local_change;
}

string PatchTreeDeletionValueElementBase::to_string() const {
    return std::to_string(get_patch_id());
}

char* PatchTreeDeletionValueElementBase::serialize(size_t* size) const {
    char* data = new char[get_SLEB128_size(std::numeric_limits<int>::max()) + sizeof(bool)];
#ifdef USE_VSI
    std::vector<uint8_t> buffer;
    encode_SLEB128(patch_id, buffer);
    std::memcpy(data, buffer.data(), buffer.size());
    *size = buffer.size();
#else
    std::memcpy(data, &patch_id, sizeof(int));
    *size = sizeof(int);
#endif
    std::memcpy(data+*size, &local_change, sizeof(bool));
    *size += sizeof(bool);
    return data;
}

size_t PatchTreeDeletionValueElementBase::deserialize(const char *data) {
    size_t consumed_size;
#ifdef USE_VSI
    patch_id = decode_SLEB128((const uint8_t*) data, &consumed_size);
#else
    std::memcpy(&patch_id, data, sizeof(int));
    consumed_size = sizeof(int);
#endif
    std::memcpy(&local_change, data+consumed_size, sizeof(bool));
    return consumed_size+sizeof(bool);
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
const T& PatchTreeDeletionValueBase<T>::get_patch_at(long index) const {
    return elements[index];
}

template <class T>
const T& PatchTreeDeletionValueBase<T>::get(int patch_id) const {
    long index = get_patchvalue_index(patch_id);
    if(index < 0 || index >= elements.size()) {
        return get_patch_at(elements.size() - 1);
        //throw std::invalid_argument("Index out of bounds (PatchTreeDeletionValue::get),"
        //                                    "tried to get patch id " + std::to_string(patch_id) + " in " +  this->to_string());
    }
    return get_patch_at(index);
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
    *size = 0;
    size_t elem_size;
#ifdef USE_VSI
    elem_size = get_SLEB128_size(std::numeric_limits<int>::max()) + sizeof(bool);
    if (T::has_positions()) {
        elem_size += PatchPositions::max_serialization_size();
    }
#else
    elem_size = sizeof(T);
#endif
    char* bytes = new char[elements.size() * elem_size];
    size_t elem_data_size;
    for (const auto& e: elements) {
        char* elem_data = e.serialize(&elem_data_size);
        std::memcpy(bytes+*size, elem_data, elem_data_size);
        *size += elem_data_size;
        delete[] elem_data;
    }
    return bytes;
}

template <class T>
void PatchTreeDeletionValueBase<T>::deserialize(const char* data, size_t size) {
    elements.clear();
    size_t offset = 0;
    while (offset < size) {
        elements.emplace_back();
        offset += elements.back().deserialize(data+offset);
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
T PatchTreeDeletionValueBase<T>::get_patch_at(long index) const {
    return elements.get_element_at(index, max_patch_id);
}

template <class T>
T PatchTreeDeletionValueBase<T>::get(int patch_id) const {
    long index = get_patchvalue_index(patch_id);
    long size = elements.size(max_patch_id);
    if(index < 0 || index >= size) {
        return get_patch_at(size - 1);
    }
    return get_patch_at(index);
}

template <class T>
bool PatchTreeDeletionValueBase<T>::is_local_change(int patch_id) const {
    if (get_size() == 0) return false;
    long index = get_patchvalue_index(patch_id);
    if (index >= 0) {
        return get_patch_at(index).is_local_change();
    }
    return get_patch_at(get_size() - 1).is_local_change();
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
    size_t size = positions_map.size() * (2*sizeof(int)+PatchPositions::max_serialization_size());
    char* data = new char[size];
    size_t offset = 0;
#ifdef USE_VSI
    std::vector<uint8_t> buffer;
    for (const auto& inter_p: positions_map) {
        encode_SLEB128(inter_p.first, buffer);
        std::memcpy(data+offset, buffer.data(), buffer.size());
        offset += buffer.size();

        buffer.clear();
        encode_SLEB128(inter_p.second.first, buffer);
        std::memcpy(data+offset, buffer.data(), buffer.size());
        offset += buffer.size();

        buffer.clear();
        size_t pos_size = 0;
        char* pos_data = inter_p.second.second.serialize(&pos_size);
        std::memcpy(data+offset, pos_data, pos_size);
        offset += PatchPositions::max_serialization_size();
    }
    return std::make_pair(data, offset);
#else
    for (const auto& inter_p: positions_map) {
        std::memcpy(data+offset, &inter_p.first, sizeof(int));
        offset += sizeof(int);
        std::memcpy(data+offset, &inter_p.second.first, sizeof(int));
        offset += sizeof(int);
        std::memcpy(data+offset, &inter_p.second.second, sizeof(PatchPositions));
        offset += sizeof(PatchPositions);
    }
    return std::make_pair(data, size);
#endif
}

void IntervalPatchPositionsContainer::deserialize(const char *data, size_t size) {
    positions_map.clear();
    auto it = positions_map.begin();
#ifdef USE_VSI
    size_t offset = 0;
    while (offset < size) {
        size_t decode_size;
        int s = decode_SLEB128((const uint8_t*)data+offset, &decode_size);
        offset += decode_size;
        int e = decode_SLEB128((const uint8_t*)data+offset, &decode_size);
        auto new_inter = positions_map.emplace_hint(it, s, std::make_pair(e,PatchPositions()));
        offset += new_inter->second.second.deserialize(data+offset);
        it = positions_map.end();
    }
#else
    for (size_t offset=0; offset<size; offset+=(2*sizeof(int) + sizeof(PatchPositions))) {
        int s, e;
        std::memcpy(&s, data+offset, sizeof(int));
        std::memcpy(&e, data+offset+sizeof(int), sizeof(int));
        PatchPositions p;
        std::memcpy(&p, data+offset+2*sizeof(int), sizeof(PatchPositions));
        positions_map.emplace_hint(it, s, std::make_pair(e,p));
        it = positions_map.end();
    }
#endif
}


void
DeltaPatchPositionsContainerBase::delta_serialize_position_vec(const vector<VerPatchPositions<int>> &position_vec, char **data,
                                                               size_t *size) {
    auto position_diff = [](char* data, const VerPatchPositions<int>& first, const VerPatchPositions<int>& second) {
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


        size_t size = sizeof(uint8_t);
        std::memcpy(data, &head, sizeof(uint8_t));
#ifdef USE_VSI
        std::vector<uint8_t> buffer;
        encode_SLEB128(second.patch_id, buffer);
        std::memcpy(data+size, buffer.data(), buffer.size());
        size += buffer.size();
        buffer.clear();
        for (PatchPosition d: diff_values) {
            encode_SLEB128(d, buffer);
            std::memcpy(data+size, buffer.data(), buffer.size());
            size += buffer.size();
            buffer.clear();
        }
#else
        size += sizeof(int) + diff_values.size()*sizeof(PatchPosition);
        std::memcpy(data+sizeof(uint8_t), &second.patch_id, sizeof(int));
        std::memcpy(data+sizeof(uint8_t)+sizeof(int), diff_values.data(), diff_values.size()*sizeof(PatchPosition));
#endif
        return size;
    };

    *size = 0;
    if (!position_vec.empty()) {
        size_t alloc_size = position_vec.size()*(PatchPositions::max_serialization_size()+sizeof(int)+sizeof(uint8_t));
        *data = new char[alloc_size];
#ifdef USE_VSI
        std::vector<uint8_t> buffer;
        encode_SLEB128(position_vec[0].patch_id, buffer);
        *size += buffer.size();
        std::memcpy(*data, buffer.data(), buffer.size());
#else
        std::memcpy(*data, &position_vec[0].patch_id, sizeof(int));
        *size += sizeof(int);
#endif
        size_t positions_data_size = 0;
        char* positions_data = position_vec[0].positions.serialize(&positions_data_size);
        std::memcpy(*data+*size, positions_data, positions_data_size);
        delete[] positions_data;
        *size += positions_data_size;
        if (position_vec.size() > 1) {
            for (size_t i=1; i<position_vec.size(); i++) {
                *size += position_diff(*data+*size, position_vec[i-1], position_vec[i]);
            }
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
#ifdef USE_VSI
        size_t leb128_decode_size = 0;
        pos.patch_id = decode_SLEB128((const uint8_t*)(data + offset), &leb128_decode_size);
        offset += leb128_decode_size;
#else
        std::memcpy(&pos.patch_id, data+offset, sizeof(int));
        offset += sizeof(int);
#endif
        PatchPosition diff = 0;
        if (head & 1) {
            offset += decode_diff(data+offset, diff);
        }
        pos.positions.sp_ = prev.sp_ + diff;

        diff = 0;
        head >>= 1;
        if (head & 1) {
            offset += decode_diff(data+offset, diff);
        }
        pos.positions.s_o = prev.s_o + diff;

        diff = 0;
        head >>= 1;
        if (head & 1) {
            offset += decode_diff(data+offset, diff);
        }
        pos.positions.s__ = prev.s__ + diff;

        diff = 0;
        head >>= 1;
        if (head & 1) {
            offset += decode_diff(data+offset, diff);
        }
        pos.positions._po = prev._po + diff;

        diff = 0;
        head >>= 1;
        if (head & 1) {
            offset += decode_diff(data+offset, diff);
        }
        pos.positions._p_ = prev._p_ + diff;

        diff = 0;
        head >>= 1;
        if (head & 1) {
            offset += decode_diff(data+offset, diff);
        }
        pos.positions.__o = prev.__o + diff;

        diff = 0;
        head >>= 1;
        if (head & 1) {
            offset += decode_diff(data+offset, diff);
        }
        pos.positions.___ = prev.___ + diff;

        return offset;
    };

    std::vector<uint8_t> s_byte;
    for (int i=0; i<size; i++) {
        s_byte.push_back(data[i]);
    }

    position_vec.resize(1);
    size_t offset = 0;
#ifdef USE_VSI
    position_vec[0].patch_id = decode_SLEB128((const uint8_t*)data, &offset);
#else
    std::memcpy(&position_vec[0].patch_id, data, sizeof(int));
    offset += sizeof(int);
#endif
    offset += position_vec[0].positions.deserialize(data+offset);
    size_t prev_index = 0;
    while(offset < size) {
        position_vec.emplace_back();  // insert default value at the back the vector to be modified
        offset += position_undiff(data+offset, position_vec[prev_index].positions, position_vec.back());
        prev_index += 1;
    }
}

size_t DeltaPatchPositionsContainerBase::decode_diff(const char *data, PatchPosition &diff) {
    size_t size = 0;
#ifdef USE_VSI
    diff = decode_SLEB128((const uint8_t*)(data), &size);
#else
    std::memcpy(&diff, data, sizeof(PatchPosition));
    size += sizeof(PatchPosition);
#endif
    return size;
}

void DeltaPatchPositionsContainerBase::serialize_position_vec(const vector<VerPatchPositions<int>> &position_vec,
                                                              char **data, size_t *size) {
    *size = position_vec.size() * sizeof(VerPatchPositions<int>);
    if (*size > 0) {
        *data = new char[*size];
        size_t offset = 0;
        for (auto& vp: position_vec) {
            std::memcpy(*data+offset, &vp.patch_id, sizeof(int));
            size_t pos_data_size = 0;
            char* pos_data = vp.positions.serialize(&pos_data_size);
            std::memcpy(*data+offset+sizeof(int), pos_data, pos_data_size);
            offset += sizeof(int) + pos_data_size;
        }
    }
}

void DeltaPatchPositionsContainerBase::deserialize_position_vec(vector<VerPatchPositions<int>> &position_vec,
                                                                const char *data, size_t size) {
    size_t n_elements = size/sizeof(VerPatchPositions<int>);
    position_vec.resize(n_elements);
    size_t offset = 0;
    for (size_t i=0; i<n_elements; i++) {
        std::memcpy(&position_vec[i].patch_id, data+offset, sizeof(int));
        offset += position_vec[i].positions.deserialize(data+offset+sizeof(int));
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
