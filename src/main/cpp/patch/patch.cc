#include <stdlib.h>
#include <iostream>
#include "patch.h"

Patch::Patch(PatchElementComparator* element_comparator) : elements(), element_comparator(element_comparator) {}

Patch::Patch(DictionaryManager* dict) : Patch(new PatchElementComparator(new PatchTreeKeyComparator(comp_s, comp_p, comp_o, dict))) {}

void Patch::add(const PatchElement& element) {
    std::vector<PatchElement>::iterator itToInsert = std::lower_bound(
            elements.begin(), elements.end(), element, element_comparator->get());
    // Overwrite existing element if triple is already present, otherwise insert new element.
    if(itToInsert != elements.end() && itToInsert->get_triple().to_string() == element.get_triple().to_string()) {
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

void Patch::overwrite(long i, const PatchElement& element) {
    elements[i] = element;
}

void Patch::addAll(const Patch& patch) {
    for(int i = 0; i < patch.get_size(); i++) {
        add(patch.get(i));
    }
}

unsigned long Patch::get_size() const {
    return elements.size();
}

const PatchElement& Patch::get(long index) const {
    if(index < 0 || index >= get_size()) {
        throw std::invalid_argument("Index out of bounds");
    }
    return elements[index];
}

inline PatchPosition contains_and_increment_position(unordered_map<long, PatchPosition>& m, long hash) {
    std::unordered_map<long, PatchPosition>::const_iterator it = m.find(hash);
    PatchPosition pos = 0;
    if (it != m.end()) {
        pos = it->second;
    }
    m[hash] = pos + 1;
    return pos;
}

PatchPositions Patch::positions(const PatchElement& element,
                                 unordered_map<long, PatchPosition>& sp_,
                                 unordered_map<long, PatchPosition>& s_o,
                                 unordered_map<long, PatchPosition>& s__,
                                 unordered_map<long, PatchPosition>& _po,
                                 unordered_map<long, PatchPosition>& _p_,
                                 unordered_map<long, PatchPosition>& __o,
                                 PatchPosition& ___) const {
    PatchPositions positions = PatchPositions();
    if(!element.is_addition() && !element.is_local_change()) {
        long hsp_ = element.get_triple().get_subject() | (element.get_triple().get_predicate() << 16);
        long hs_o = element.get_triple().get_subject() | (element.get_triple().get_object() << 16);
        long hs__ = element.get_triple().get_subject();
        long h_po = element.get_triple().get_predicate() | (element.get_triple().get_object() << 16);
        long h_p_ = element.get_triple().get_predicate();
        long h__o = element.get_triple().get_object();

        positions.sp_ = contains_and_increment_position(sp_, hsp_);
        positions.s_o = contains_and_increment_position(s_o, hs_o);
        positions.s__ = contains_and_increment_position(s__, hs__);
        positions._po = contains_and_increment_position(_po, h_po);
        positions._p_ = contains_and_increment_position(_p_, h_p_);
        positions.__o = contains_and_increment_position(__o, h__o);
        positions.___ = ___++;
    }
    return positions;
}

PatchPosition Patch::position_of_pattern(const PatchElement& element, bool s, bool p, bool o, bool type) const {
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

PatchPosition Patch::position_of(const PatchElement& element) const {
    std::vector<PatchElement>::const_iterator findIt = std::lower_bound(elements.begin(), elements.end(), element,
                                                                        element_comparator->get());
    return std::distance(elements.begin(), findIt);
}

PatchPosition Patch::position_of_strict(const PatchElement& element) const {
    std::vector<PatchElement>::const_iterator findIt = std::lower_bound(elements.begin(), elements.end(), element,
                                                                        element_comparator->get());
    if (findIt != elements.end() && *findIt == element) {
        return std::distance(elements.begin(), findIt);
    }
    return -1;
}

string Patch::to_string() const {
    string ret;
    for(int i = 0; i < elements.size(); i++) {
        ret += elements[i].to_string();
        if(elements[i].is_local_change()) {
            ret += " L";
        }
        ret += "\n";
    }
    return ret;
}

string Patch::to_string(Dictionary& dict) const {
    string ret;
    for(int i = 0; i < elements.size(); i++) {
        ret += elements[i].to_string(dict);
        if(elements[i].is_local_change()) {
            ret += " L";
        }
        ret += "\n";
    }
    return ret;
}

long Patch::index_of_triple(const Triple& triple) const {
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
