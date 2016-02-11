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
