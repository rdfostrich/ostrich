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
