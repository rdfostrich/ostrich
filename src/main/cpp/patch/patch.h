#ifndef TPFPATCH_STORE_PATCH_H
#define TPFPATCH_STORE_PATCH_H

#include <string>

using namespace std;

// S, P, O holder
typedef struct Triple {
    string subject;
    string predicate;
    string object;
    Triple(string subject, string predicate, string object) :
            subject(subject), predicate(predicate), object(object) {}
} Triple; // TODO: use dictionary

// A triple annotated with addition or deletion
typedef struct PatchElement {
    Triple triple;
    bool addition;
    PatchElement(Triple triple, bool addition) : triple(triple), addition(addition) {}
} PatchElement;

// A linked list of PatchElements
typedef struct PatchElements {
    PatchElement patchElement;
    PatchElements* next;
    PatchElements(PatchElement patchElement) : patchElement(patchElement) {}
} PatchElements;

// A key in the PatchTree is a triple
typedef Triple PatchTreeKey;

// A value in the PatchTree is a linked list of patch_id->patch_position
typedef struct PatchTreeValue {
    int patch_id;
    int patch_position;
    PatchTreeValue* next;
} PatchTreeValue;

#endif //TPFPATCH_STORE_PATCH_H
