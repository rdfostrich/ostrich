#ifndef TPFPATCH_STORE_TRIPLEDELTAITERATOR_H
#define TPFPATCH_STORE_TRIPLEDELTAITERATOR_H

#include "../patch/triple.h"
#include "../patch/patch_tree_iterator.h"
#include "../patch/patch_tree.h"

// Triple annotated with addition/deletion.
class TripleDelta {
protected:
    Triple* triple;
    bool addition;
public:
    TripleDelta();
    TripleDelta(Triple* triple, bool addition);
    ~TripleDelta();
    Triple* get_triple();
    bool is_addition();
    void set_addition(bool addition);
};

// Iterator for triples annotated with addition/deletion.
class TripleDeltaIterator {
public:
    virtual ~TripleDeltaIterator() = 0;
    virtual bool next(TripleDelta* triple) = 0;
    size_t get_count();
    TripleDeltaIterator* offset(int offset);
};

// Triple delta iterator where elements from a single patch are emitted.
template <class DV>
class ForwardPatchTripleDeltaIterator : public TripleDeltaIterator {
protected:
    PatchTreeIteratorBase<DV>* it;
    PatchTreeValueBase<DV>* value;
public:
    ForwardPatchTripleDeltaIterator(PatchTree* patchTree, const Triple &triple_pattern, int patch_id_end);
    ~ForwardPatchTripleDeltaIterator();
    bool next(TripleDelta* triple);
};

// Triple delta iterator where elements only differences between two patches are emitted.
template <class DV>
class FowardDiffPatchTripleDeltaIterator : public ForwardPatchTripleDeltaIterator<DV> {
protected:
    int patch_id_start;
    int patch_id_end;
public:
    FowardDiffPatchTripleDeltaIterator(PatchTree* patchTree, const Triple &triple_pattern, int patch_id_start, int patch_id_end);
    bool next(TripleDelta* triple);
};

class EmptyTripleDeltaIterator : public TripleDeltaIterator {
public:
    bool next(TripleDelta* triple);
};


#endif //TPFPATCH_STORE_TRIPLEDELTAITERATOR_H
