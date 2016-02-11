#ifndef TPFPATCH_STORE_PATCH_ELEMENT_H
#define TPFPATCH_STORE_PATCH_ELEMENT_H

#include "triple.h"

// A triple annotated with addition or deletion
class PatchElement {
protected:
    Triple triple;
    bool addition;
public:
    PatchElement(Triple triple, bool addition);
    Triple get_triple();
    bool is_addition();
    string to_string();
};


#endif //TPFPATCH_STORE_PATCH_ELEMENT_H
