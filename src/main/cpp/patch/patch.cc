#include <cstdlib>
#include <iostream>
#include "patch.h"

void Patch::addAll(const Patch &patch) {
    PatchIterator* it = patch.iterator();
    while (it->has_next()) {
        add(it->next());
    }
    delete it;
}

string Patch::to_string() const {
    string ret;
    PatchIterator* it = iterator();
    while (it->has_next()) {
        const PatchElement& element = it->next();
        ret += element.to_string();
        if(element.is_local_change()) {
            ret += " L";
        }
        ret += "\n";
    }
    delete it;
    return ret;
}

string Patch::to_string(hdt::Dictionary& dict) const {
    string ret;
    PatchIterator* it = iterator();
    while (it->has_next()) {
        const PatchElement& element = it->next();
        ret += element.to_string(dict);
        if(element.is_local_change()) {
            ret += " L";
        }
        ret += "\n";
    }
    delete it;
    return ret;
}

PatchSorted::PatchSorted(PatchElementComparator* element_comparator) : elements(), element_comparator(element_comparator) {}

PatchSorted::PatchSorted(std::shared_ptr<DictionaryManager> dict) : PatchSorted(new PatchElementComparator(new PatchTreeKeyComparator(comp_s, comp_p, comp_o, dict))) {}

PatchSorted::PatchSorted(PatchElementComparator* element_comparator, std::vector<PatchElement> elements) : elements(elements), element_comparator(element_comparator) {}

void PatchSorted::add(const PatchElement& element) {
    std::vector<PatchElement>::iterator itToInsert = std::lower_bound(
            elements.begin(), elements.end(), element, element_comparator->get());
    // Overwrite existing element if triple is already present, otherwise insert new element.
    if(itToInsert != elements.end() && itToInsert->get_triple() == element.get_triple()) {
        bool was_addition_change = itToInsert->is_addition() != element.is_addition();
        bool was_local_change = itToInsert->is_local_change();
        *itToInsert = element;
        if (was_addition_change) {
            itToInsert->set_local_change(!was_local_change);
        }
    } else {
        elements.insert(itToInsert, element);
    }
}

void PatchSorted::add_unsorted(const PatchElement &element) {
    elements.push_back(element);
}

void PatchSorted::sort() {
    std::sort(elements.begin(), elements.end(), element_comparator->get());
}

void PatchSorted::overwrite(long i, const PatchElement& element) {
    elements[i] = element;
}

unsigned long PatchSorted::get_size() const {
    return elements.size();
}

const PatchElement& PatchSorted::get(long index) const {
    if(index < 0 || index >= get_size()) {
        throw std::invalid_argument("Index out of bounds");
    }
    return elements[index];
}

const std::vector<PatchElement>& PatchSorted::get_vector() const {
    return elements;
}

PatchIterator* PatchSorted::iterator() const {
    return new PatchIteratorVector(elements.cbegin(), elements.cend());
}

inline PatchPosition contains_and_increment_position(kyotocabinet::HashDB& m, size_t hash) {
    size_t _;
    char raw_key[sizeof(long)];
    char* raw_value;
    bool was_present = false;
    memcpy(raw_key, &hash, sizeof(long));

    raw_value = m.get(raw_key, sizeof(long), &_);
    PatchPosition pos = 0;
    if (raw_value != nullptr) {
        was_present = true;
        memcpy(&pos, raw_value, sizeof(PatchPosition));
    } else {
        raw_value = (char*) malloc(sizeof(long));
    }
    pos++;
    memcpy(raw_value, &pos, sizeof(PatchPosition));
    m.set(raw_key, sizeof(long), raw_value, sizeof(PatchPosition));

    if (was_present) {
        delete[] raw_value;
    } else {
        free(raw_value);
    }
    return pos - 1;
}

PatchPositions Patch::positions(const Triple& triple,
                                kyotocabinet::HashDB& sp_,
                                kyotocabinet::HashDB& s_o,
                                kyotocabinet::HashDB& s__,
                                kyotocabinet::HashDB& _po,
                                kyotocabinet::HashDB& _p_,
                                kyotocabinet::HashDB& __o,
                                PatchPosition& ___) {
    PatchPositions positions = PatchPositions();
    size_t hsp_ = triple.get_subject() | (triple.get_predicate() << 16);
    size_t hs_o = triple.get_subject() | (triple.get_object() << 16);
    size_t hs__ = triple.get_subject();
    size_t h_po = triple.get_predicate() | (triple.get_object() << 16);
    size_t h_p_ = triple.get_predicate();
    size_t h__o = triple.get_object();

    positions.sp_ = contains_and_increment_position(sp_, hsp_);
    positions.s_o = contains_and_increment_position(s_o, hs_o);
    positions.s__ = contains_and_increment_position(s__, hs__);
    positions._po = contains_and_increment_position(_po, h_po);
    positions._p_ = contains_and_increment_position(_p_, h_p_);
    positions.__o = contains_and_increment_position(__o, h__o);
    positions.___ = ___++;

    // Flush db to disk
    if (!(___ % FLUSH_POSITIONS_COUNT)) {
        sp_.synchronize();
        s_o.synchronize();
        s__.synchronize();
        _po.synchronize();
        _p_.synchronize();
        __o.synchronize();
    }

    return positions;
}

PatchPosition PatchSorted::position_of_pattern(const PatchElement& element, bool s, bool p, bool o, bool type) const {
    // First find the position of the element in O(log n)
    std::vector<PatchElement>::const_iterator findIt = std::lower_bound(elements.begin(), elements.end(), element,
                                                                        element_comparator->get());
    // Count the matching patch elements from this position to the beginning
    int position = -1;
    while(findIt >= elements.begin()) {
        PatchElement matching = *findIt;
        if((!s || matching.get_triple().get_subject() == element.get_triple().get_subject())
           && (!p || matching.get_triple().get_predicate() == element.get_triple().get_predicate())
           && (!o || matching.get_triple().get_object() == element.get_triple().get_object())
           && (!type || matching.is_addition() == element.is_addition())) {
            position++;
        }
        findIt--;
    };
    return position;
}

PatchPosition PatchSorted::position_of(const PatchElement& element) const {
    std::vector<PatchElement>::const_iterator findIt = std::lower_bound(elements.begin(), elements.end(), element,
                                                                        element_comparator->get());
    return std::distance(elements.begin(), findIt);
}

PatchPosition PatchSorted::position_of_strict(const PatchElement& element) const {
    std::vector<PatchElement>::const_iterator findIt = std::lower_bound(elements.begin(), elements.end(), element,
                                                                        element_comparator->get());
    if (findIt != elements.end() && *findIt == element) {
        return std::distance(elements.begin(), findIt);
    }
    return -1;
}

long PatchSorted::index_of_triple(const Triple& triple) const {
    PatchElement element1(triple, false);
    std::vector<PatchElement>::const_iterator findIt1 = std::lower_bound(elements.begin(), elements.end(), element1,
                                                                         element_comparator->get());
    if(findIt1 != elements.end() && *findIt1 == element1) {
        return std::distance(elements.begin(), findIt1);
    }

    PatchElement element2(triple, true);
    std::vector<PatchElement>::const_iterator findIt2 = std::lower_bound(elements.begin(), elements.end(), element2,
                                                                         element_comparator->get());
    if(findIt2 != elements.end() && *findIt2 == element2) {
        return std::distance(elements.begin(), findIt2);
    }
    return -1;
}

PatchElementIterator *PatchSorted::element_iterator() {
    return nullptr;
}

void PatchUnsorted::add(const PatchElement &element) {
    elements.push_back(element);
}

unsigned long PatchUnsorted::get_size() const {
    return elements.size();
}

const PatchElement &PatchUnsorted::get(long index) const {
    if(index < 0 || index >= get_size()) {
        throw std::invalid_argument("Index out of bounds");
    }
    return elements[index];
}

PatchIterator* PatchUnsorted::iterator() const {
    return new PatchIteratorVector(elements.cbegin(), elements.cend());
}

const std::vector<PatchElement>& PatchUnsorted::get_vector() const {
    return elements;
}

void PatchHashed::add(const PatchElement &element) {
    elements.insert(std::pair<Triple, std::pair<bool, bool>>(element.get_triple(), std::pair<bool, bool>(element.is_addition(), element.is_local_change())));
}

unsigned long PatchHashed::get_size() const {
    return elements.size();
}

PatchIterator* PatchHashed::iterator() const {
    return new PatchIteratorHashed(elements.cbegin(), elements.cend());
}

PatchSorted* PatchHashed::join_sorted(const PatchIndexed &patch, PatchElementComparator *element_comparator) {
    // If the reconstructed patch (this) is empty, simply use vector from given patch and sort that.
    if (get_size() == 0) {
        PatchSorted* new_patch = new PatchSorted(element_comparator, patch.get_vector());
        new_patch->sort();
        return new_patch;
    }

    PatchSorted* new_patch = new PatchSorted(element_comparator);
    std::unordered_set<Triple> skip_elements;

    // Optimized bulk insertion of new patch
    PatchIterator* it = patch.iterator();
    while (it->has_next()) {
        const PatchElement& element = it->next();
        auto existing_element = elements.find(element.get_triple());
        if (existing_element != elements.end()) {
            PatchElement new_element(element);
            bool was_addition_change = existing_element->second.first != element.is_addition();
            bool was_local_change = existing_element->second.second;
            if (was_addition_change) {
                new_element.set_local_change(!was_local_change);
            }
            new_patch->add_unsorted(new_element);
            skip_elements.insert(new_element.get_triple());
        } else {
            new_patch->add_unsorted(element);
        }
    }
    delete it;

    // Add all elements from this patch that haven't been added yet
    for (const auto& element : elements) {
        if (skip_elements.find(element.first) == skip_elements.end()) {
            new_patch->add_unsorted(PatchElement(element.first, element.second.first, element.second.second));
        }
    }

    // Sort afterwards instead of always maintaining a sorted state in the vector.
    new_patch->sort();
    return new_patch;
}

PatchIteratorVector::PatchIteratorVector(std::vector<PatchElement>::const_iterator it, std::vector<PatchElement>::const_iterator it_end) : it(it), it_end(it_end) {}
PatchIteratorVector::~PatchIteratorVector() = default;

bool PatchIteratorVector::has_next() {
    return it != it_end;
}

const PatchElement PatchIteratorVector::next() {
    return *(it++);
}

PatchIteratorHashed::PatchIteratorHashed(std::unordered_map<Triple, std::pair<bool, bool>>::const_iterator it, std::unordered_map<Triple, std::pair<bool, bool>>::const_iterator it_end) : it(it), it_end(it_end) {}
PatchIteratorHashed::~PatchIteratorHashed() = default;

bool PatchIteratorHashed::has_next() {
    return it != it_end;
}

const PatchElement PatchIteratorHashed::next() {
    std::pair<Triple, std::pair<bool, bool>> data = *(it++);
    return PatchElement(data.first, data.second.first, data.second.second);
}
