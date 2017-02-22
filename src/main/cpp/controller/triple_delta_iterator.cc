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

TripleDeltaIterator::~TripleDeltaIterator() {}

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

template <class DV>
ForwardPatchTripleDeltaIterator<DV>::ForwardPatchTripleDeltaIterator(PatchTree* patchTree, const Triple &triple_pattern, int patch_id_end) : it(patchTree->iterator<DV>(&triple_pattern)) {
    it->set_patch_filter(patch_id_end, true);
    it->set_filter_local_changes(true);
    it->set_early_break(true);
    value = new PatchTreeValueBase<DV>();
}

template <class DV>
ForwardPatchTripleDeltaIterator<DV>::~ForwardPatchTripleDeltaIterator() {
    delete it;
    delete value;
}

template <class DV>
bool ForwardPatchTripleDeltaIterator<DV>::next(TripleDelta* triple) {
    bool ret = it->next(triple->get_triple(), value);
    triple->set_addition(value->is_addition(it->get_patch_id_filter(), true));
    return ret;
}

template <class DV>
FowardDiffPatchTripleDeltaIterator<DV>::FowardDiffPatchTripleDeltaIterator(PatchTree* patchTree, const Triple &triple_pattern, int patch_id_start, int patch_id_end)
        : ForwardPatchTripleDeltaIterator<DV>(patchTree, triple_pattern, patch_id_end), patch_id_start(patch_id_start), patch_id_end(patch_id_end) {
    this->it->reset_patch_filter();
    this->it->set_filter_local_changes(false);
}

template <class DV>
bool FowardDiffPatchTripleDeltaIterator<DV>::next(TripleDelta *triple) {
    bool valid;
    while ((valid = this->it->next(triple->get_triple(), this->value))
                    && this->value->is_delta_type_equal(patch_id_start, patch_id_end)) {}
    if (valid) {
        triple->set_addition(this->value->is_addition(patch_id_end, true));
    }
    return valid;
}

template class ForwardPatchTripleDeltaIterator<PatchTreeDeletionValue>;
template class ForwardPatchTripleDeltaIterator<PatchTreeDeletionValueReduced>;
template class FowardDiffPatchTripleDeltaIterator<PatchTreeDeletionValue>;
template class FowardDiffPatchTripleDeltaIterator<PatchTreeDeletionValueReduced>;
