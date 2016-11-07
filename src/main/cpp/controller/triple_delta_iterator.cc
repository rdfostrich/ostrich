#include "triple_delta_iterator.h"

TripleDelta::TripleDelta() : triple(new Triple()), addition(true) {}

TripleDelta::TripleDelta(Triple* triple, bool addition) : triple(triple), addition(addition) {}

Triple* TripleDelta::get_triple() {
    return triple;
}

bool TripleDelta::is_addition() {
    return addition;
}

TripleDelta::~TripleDelta() {
    delete triple;
}

void TripleDelta::set_addition(bool addition) {
    this->addition = addition;
}

TripleDeltaIterator* TripleDeltaIterator::offset(int offset) {
    TripleDelta td;
    while(offset-- > 0 && next(&td));
    return this;
}

size_t TripleDeltaIterator::get_count() {
    size_t count = 0;
    TripleDelta td;
    while (next(&td)) count++;
    return count;
}

bool EmptyTripleDeltaIterator::next(TripleDelta *triple) {
    return false;
}

ForwardPatchTripleDeltaIterator::ForwardPatchTripleDeltaIterator(PatchTreeIterator* it) : it(it) {
    value = new PatchTreeValue();
}

ForwardPatchTripleDeltaIterator::~ForwardPatchTripleDeltaIterator() {
    delete it;
    delete value;
}

bool ForwardPatchTripleDeltaIterator::next(TripleDelta* triple) {
    bool ret = it->next(triple->get_triple(), value);
    triple->set_addition(value->is_addition(it->get_patch_id_filter(), true));
    return ret;
}

FowardDiffPatchTripleDeltaIterator::FowardDiffPatchTripleDeltaIterator(PatchTreeIterator *it, int patch_id_start,
                                                                       int patch_id_end)
        : ForwardPatchTripleDeltaIterator(it), patch_id_start(patch_id_start), patch_id_end(patch_id_end) {}

bool FowardDiffPatchTripleDeltaIterator::next(TripleDelta *triple) {
    bool valid;
    while ((valid = it->next(triple->get_triple(), value))
                    && value->is_delta_type_equal(patch_id_start, patch_id_end)) {}
    if (valid) {
        triple->set_addition(value->is_addition(patch_id_end, true));
    }
    return valid;
}
