#include <stdlib.h>
#include <iostream>
#include "patch.h"

Patch::Patch() : elements() {}

void Patch::add(PatchElement element) {
    std::vector<PatchElement>::iterator itToInsert = std::lower_bound(
            elements.begin(), elements.end(), element);
    elements.insert(itToInsert, element);
}

void Patch::addAll(Patch patch) {
    for(int i = 0; i < patch.get_size(); i++) {
        add(patch.get(i));
    }
}

unsigned long Patch::get_size() {
    return elements.size();
}

PatchElement Patch::get(int index) {
    if(index < 0 || index >= get_size()) {
        throw std::invalid_argument("Index out of bounds");
    }
    return elements[index];
}

PatchPositions Patch::positions(PatchElement element) {
    PatchPositions positions = PatchPositions();
    // First find the position of the element in O(log n)
    std::vector<PatchElement>::iterator findIt = std::lower_bound(elements.begin(), elements.end(), element);
    // Count the matching patch elements from this position to the beginning for all triple patterns
    while(findIt >= elements.begin()) {
        PatchElement matching = *findIt;

        bool s = matching.get_triple().get_subject() == element.get_triple().get_subject();
        bool p = matching.get_triple().get_predicate() == element.get_triple().get_predicate();
        bool o = matching.get_triple().get_object() == element.get_triple().get_object();

        if(s && p) positions.sp_++;
        if(s && o) positions.s_o++;
        if(s) positions.s__++;
        if(p && o) positions._po++;
        if(p) positions._p_++;
        if(o) positions.__o++;
        positions.___++;

        findIt--;
    };
    return positions;
}

PatchPosition Patch::position_of_pattern(PatchElement element, bool s, bool p, bool o, bool type) {
    // First find the position of the element in O(log n)
    std::vector<PatchElement>::iterator findIt = std::lower_bound(elements.begin(), elements.end(), element);
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

PatchPosition Patch::position_of(PatchElement element) {
    std::vector<PatchElement>::iterator findIt = std::lower_bound(elements.begin(), elements.end(), element);
    return std::distance(elements.begin(), findIt);
}

PatchPosition Patch::position_of_strict(PatchElement element) {
    std::vector<PatchElement>::iterator findIt = std::lower_bound(elements.begin(), elements.end(), element);
    if (findIt != elements.end() && *findIt == element) {
        return std::distance(elements.begin(), findIt);
    }
    return -1;
}

string Patch::to_string() {
    string ret;
    for(int i = 0; i < elements.size(); i++) {
        ret += elements[i].to_string() + "\n";
    }
    return ret;
}
