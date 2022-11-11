#include "patch_element.h"

PatchElement::PatchElement() : triple(Triple()), addition(false), local_change(false) {}
PatchElement::PatchElement(const Triple& triple, bool addition) : triple(triple), addition(addition), local_change(false) {}
PatchElement::PatchElement(const Triple& triple, bool addition, bool local_change) : triple(triple), addition(addition), local_change(local_change) {}

void PatchElement::set_triple(const Triple triple) {
    this->triple = triple;
}

const Triple& PatchElement::get_triple() const {
    return triple;
}

void PatchElement::set_addition(bool addition) {
    this->addition = addition;
}

bool PatchElement::is_addition() const {
    return addition;
}

void PatchElement::set_local_change(bool local_change) {
    this->local_change = local_change;
}

bool PatchElement::is_local_change() const {
    return local_change;
}

const string PatchElement::to_string() const {
    return get_triple().to_string() + " (" + (is_addition() ? "+" : "-") + ")";
}

const string PatchElement::to_string(hdt::Dictionary& dict) const {
    return get_triple().to_string(dict) + " (" + (is_addition() ? "+" : "-") + ")";
}
