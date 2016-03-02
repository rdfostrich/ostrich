#include "patch_element.h"

PatchElement::PatchElement(Triple triple, bool addition) : triple(triple), addition(addition), local_change(false) {}

Triple PatchElement::get_triple() {
    return triple;
}

bool PatchElement::is_addition() {
    return addition;
}

void PatchElement::set_local_change(bool local_change) {
    this->local_change = local_change;
}

bool PatchElement::is_local_change() {
    return local_change;
}

string PatchElement::to_string() {
    return triple.to_string() + " (" + (addition ? "+" : "-") + ")";
}
