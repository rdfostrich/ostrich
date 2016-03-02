#include <stdlib.h>
#include <iostream>
#include "patch.h"

Patch::Patch() : elements() {}

void Patch::add(PatchElement element) {
    std::vector<PatchElement>::iterator itToInsert = std::lower_bound(
            elements.begin(), elements.end(), element);
    // Overwrite existing element if triple is already present, otherwise insert new element.
    if(itToInsert != elements.end() && itToInsert->get_triple().to_string() == element.get_triple().to_string()
       && itToInsert->is_addition() == element.is_addition() && itToInsert->is_local_change() == element.is_local_change()) {
        *itToInsert = element;
    } else {
        elements.insert(itToInsert, element);
    }
}

void Patch::overwrite(long i, PatchElement element) {
    elements[i] = element;
}

void Patch::addAll(Patch patch) {
    for(int i = 0; i < patch.get_size(); i++) {
        add(patch.get(i));
    }
}

unsigned long Patch::get_size() {
    return elements.size();
}

PatchElement Patch::get(long index) {
    if(index < 0 || index >= get_size()) {
        throw std::invalid_argument("Index out of bounds");
    }
    return elements[index];
}

PatchPositions Patch::positions(PatchElement element) {
    PatchPositions positions = PatchPositions();
    if(!element.is_addition()) {
        // First find the position of the element in O(log n)
        std::vector<PatchElement>::iterator findIt = std::lower_bound(elements.begin(), elements.end(), element);
        // Count the matching patch elements from this position to the beginning for all triple patterns
        while (findIt >= elements.begin()) {
            PatchElement matching = *findIt;
            // For additions, we don't have to store the relative positions.
            if(!matching.is_addition() && !matching.is_local_change()) {
                bool s = matching.get_triple().get_subject() == element.get_triple().get_subject();
                bool p = matching.get_triple().get_predicate() == element.get_triple().get_predicate();
                bool o = matching.get_triple().get_object() == element.get_triple().get_object();

                if (s && p) positions.sp_++;
                if (s && o) positions.s_o++;
                if (s) positions.s__++;
                if (p && o) positions._po++;
                if (p) positions._p_++;
                if (o) positions.__o++;
                positions.___++;
            }
            findIt--;
        };
    }
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
        ret += elements[i].to_string();
        if(elements[i].is_local_change()) {
            ret += " L";
        }
        ret += "\n";
    }
    return ret;
}

long Patch::index_of_triple(Triple triple) {
    PatchElement element1(triple, false);
    std::vector<PatchElement>::iterator findIt1 = std::find(elements.begin(), elements.end(), element1);
    if(findIt1 != elements.end()) {
        return std::distance(elements.begin(), findIt1);
    }

    PatchElement element2(triple, true);
    std::vector<PatchElement>::iterator findIt2 = std::find(elements.begin(), elements.end(), element2);
    if(findIt2 != elements.end()) {
        return std::distance(elements.begin(), findIt2);
    }
    return -1;
}

Patch Patch::apply_local_changes() {
    Patch newPatch;
    for(int i = 0; i < get_size(); i++) {
        PatchElement patchElement = get(i);
        long existing_index = newPatch.index_of_triple(patchElement.get_triple());
        if(existing_index >= 0) {
            PatchElement existingElement = newPatch.get(existing_index);
            if(existingElement.is_addition() != patchElement.is_addition() && !patchElement.is_local_change()) {
                patchElement.set_local_change(!existingElement.is_local_change());
                newPatch.overwrite(existing_index, patchElement);
            }
        } else {
            newPatch.add(patchElement);
        }
    }
    return newPatch;
}
