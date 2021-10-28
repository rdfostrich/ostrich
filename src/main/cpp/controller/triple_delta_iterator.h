#ifndef TPFPATCH_STORE_TRIPLEDELTAITERATOR_H
#define TPFPATCH_STORE_TRIPLEDELTAITERATOR_H

#include "../patch/triple.h"
#include "../patch/patch_tree_iterator.h"
#include "../patch/patch_tree.h"
#include "../snapshot/snapshot_manager.h"
#include "controller.h"

// Triple annotated with addition/deletion.
class TripleDelta {
protected:
    Triple* triple;
    bool addition;
    DictionaryManager *dict;
public:
    TripleDelta();
    TripleDelta(Triple* triple, bool addition);
    ~TripleDelta();
    Triple* get_triple();
    bool is_addition();
    void set_addition(bool addition);
    DictionaryManager* get_dictionary();
    void set_dictionary(DictionaryManager *dictionary);
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




class SnapshotDiffIterator: public TripleDeltaIterator {
private:
    IteratorTripleString* snapshot_it_1;
    IteratorTripleString* snapshot_it_2;
    TripleString* t1;
    TripleString* t2;

    DictionaryManager* dict1;
    DictionaryManager* dict2;

    static int compare_ts(TripleString* ts1, TripleString* ts2);

public:
    SnapshotDiffIterator(TripleString& triple_pattern, SnapshotManager* manager , int snapshot_1, int snapshot_2);
    bool next(TripleDelta* triple) override;
};


class SortedTripleDeltaIterator: public TripleDeltaIterator {
private:
    size_t index;
    std::vector<TripleDelta> triples;

public:
    SortedTripleDeltaIterator(TripleDeltaIterator* iterator);
    bool next(TripleDelta* triple) override;
};


int compare_triple_delta(TripleDelta *td1, TripleDelta *td2);

// Assume that the input iterators are sorted
class MergeDiffIterator: public TripleDeltaIterator {
private:
    TripleDeltaIterator* it1;
    TripleDeltaIterator* it2;
    bool status1;
    bool status2;
    TripleDelta* triple1;
    TripleDelta* triple2;

public:
    MergeDiffIterator(TripleDeltaIterator* iterator_1, TripleDeltaIterator* iterator_2);
    bool next(TripleDelta* triple) override;
};



#endif //TPFPATCH_STORE_TRIPLEDELTAITERATOR_H
