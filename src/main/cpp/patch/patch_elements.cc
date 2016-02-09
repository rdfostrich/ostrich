#include <stdlib.h>
#include <iostream>
#include "patch_elements.h"

PatchElements::PatchElements() : elements(NULL), amount(0) {}

PatchElements::~PatchElements() {
    free(elements);
}

void PatchElements::add(PatchElement element) {
    if(elements) {
        elements = (PatchElement *) realloc(elements, sizeof(PatchElement) * (amount + 1));
    } else {
        elements = (PatchElement *) malloc(sizeof(PatchElement));
    }
    elements[amount++] = element;
}

int PatchElements::getSize() {
    return amount;
}

PatchElement PatchElements::get(int index) {
    if(index < 0 || index >= getSize()) {
        throw std::invalid_argument("Index out of bounds");
    }
    return elements[index];
}
