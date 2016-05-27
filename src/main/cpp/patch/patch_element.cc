#include "patch_element.h"

PatchElement::PatchElement(const Triple& triple, bool addition) : triple(triple), addition(addition), local_change(false) {}

const Triple& PatchElement::get_triple() const {
    return triple;
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
