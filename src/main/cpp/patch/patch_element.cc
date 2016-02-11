#include "patch_element.h"

PatchElement::PatchElement(Triple triple, bool addition) : triple(triple), addition(addition) {}

Triple PatchElement::get_triple() {
    return triple;
}

bool PatchElement::is_addition() {
    return addition;
}

string PatchElement::to_string() {
    return triple.to_string() + " (" + (addition ? "+" : "-") + ")";
}
