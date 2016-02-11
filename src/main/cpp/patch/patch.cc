#include <stdlib.h>
#include <iostream>
#include "patch.h"

Patch::Patch() : elements() {}

void Patch::add(PatchElement element) {
    elements.push_back(element);
}

unsigned long Patch::getSize() {
    return elements.size();
}

PatchElement Patch::get(int index) {
    if(index < 0 || index >= getSize()) {
        throw std::invalid_argument("Index out of bounds");
    }
    return elements[index];
}

string Patch::to_string() {
    string ret;
    for(int i = 0; i < elements.size(); i++) {
        ret += elements[i].to_string() + "\n";
    }
    return ret;
}
