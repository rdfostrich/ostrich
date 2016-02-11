#include <stdlib.h>
#include <iostream>
#include "patch_elements.h"

PatchElements::PatchElements() : elements() {}

void PatchElements::add(PatchElement element) {
    elements.push_back(element);
}

unsigned long PatchElements::getSize() {
    return elements.size();
}

PatchElement PatchElements::get(int index) {
    if(index < 0 || index >= getSize()) {
        throw std::invalid_argument("Index out of bounds");
    }
    return elements[index];
}

string PatchElements::to_string() {
    string ret;
    for(int i = 0; i < elements.size(); i++) {
        ret += elements[i].to_string() + "\n";
    }
    return ret;
}
