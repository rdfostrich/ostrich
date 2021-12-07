#ifndef TPFPATCH_STORE_TRIPLEDELTAITERATOR_H
#define TPFPATCH_STORE_TRIPLEDELTAITERATOR_H

#include <unordered_set>

#include "../patch/triple.h"
#include "../patch/patch_tree_iterator.h"
#include "../patch/patch_tree.h"
#include "../snapshot/snapshot_manager.h"
#include "../patch/patch_tree_manager.h"

// Triple annotated with addition/deletion.
class TripleDelta {
protected:
    Triple* triple;
    bool addition;
    std::shared_ptr<DictionaryManager> dict;
public:
    TripleDelta();
    TripleDelta(Triple* triple, bool addition);
    ~TripleDelta();
    Triple* get_triple();
    bool is_addition();
    void set_addition(bool addition);
    std::shared_ptr<DictionaryManager> get_dictionary();
    void set_dictionary(std::shared_ptr<DictionaryManager> dictionary);
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
    std::shared_ptr<DictionaryManager> dict;
public:
    ForwardPatchTripleDeltaIterator(std::shared_ptr<PatchTree> patchTree, const Triple &triple_pattern, int patch_id_end, std::shared_ptr<DictionaryManager> dict);
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
    FowardDiffPatchTripleDeltaIterator(std::shared_ptr<PatchTree> patchTree, const Triple &triple_pattern, int patch_id_start, int patch_id_end, std::shared_ptr<DictionaryManager> dict);
    bool next(TripleDelta* triple);
};

class EmptyTripleDeltaIterator : public TripleDeltaIterator {
public:
    bool next(TripleDelta* triple);
};



// Snapshots should emit in SPO order
// This should always be the case with our system
class SnapshotDiffIterator: public TripleDeltaIterator {
private:
    IteratorTripleString* snapshot_it_1;
    IteratorTripleString* snapshot_it_2;
    TripleString* t1;
    TripleString* t2;

    std::shared_ptr<DictionaryManager> dict1;
    std::shared_ptr<DictionaryManager> dict2;

    static int compare_ts(TripleString* ts1, TripleString* ts2);

public:
    SnapshotDiffIterator(const StringTriple& triple_pattern, SnapshotManager* manager , int snapshot_1, int snapshot_2);
    ~SnapshotDiffIterator();
    bool next(TripleDelta* triple) override;
};

class IterativeSnapshotDiffIterator: public TripleDeltaIterator {
private:
    TripleDeltaIterator *internal_it;

public:
    IterativeSnapshotDiffIterator(const StringTriple &triple_pattern, SnapshotManager *snapshot_manager,
                                  PatchTreeManager *patch_tree_manager, int snapshot_id_1, int snapshot_id_2);
    ~IterativeSnapshotDiffIterator() override;
    bool next(TripleDelta* triple) override;
};

class AutoSnapshotDiffIterator: public TripleDeltaIterator {
private:
    TripleDeltaIterator *internal_it;

public:
    AutoSnapshotDiffIterator(const StringTriple &triple_pattern, SnapshotManager *snapshot_manager,
                             PatchTreeManager *patch_tree_manager, int snapshot_id_1, int snapshot_id_2);
    ~AutoSnapshotDiffIterator() override;
    bool next(TripleDelta* triple) override;
};


// Sort a TripleDeltaIterator in SPO order
class SortedTripleDeltaIterator: public TripleDeltaIterator {
private:
    size_t index;
    std::vector<TripleDelta*> triples;

public:
    explicit SortedTripleDeltaIterator(TripleDeltaIterator* iterator);
    ~SortedTripleDeltaIterator() override;
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
    ~MergeDiffIterator() override;
    bool next(TripleDelta* triple) override;
};

// Special case of merge
// where we want to merge the diff of two snapshots
// and the diff of a patch relative to the first snapshot
class MergeDiffIteratorCase2: public TripleDeltaIterator {
private:
    TripleDeltaIterator* it1;
    TripleDeltaIterator* it2;
    bool status1;
    bool status2;
    TripleDelta* triple1;
    TripleDelta* triple2;

public:
    MergeDiffIteratorCase2(TripleDeltaIterator* iterator_1, TripleDeltaIterator* iterator_2);
    ~MergeDiffIteratorCase2() override;
    bool next(TripleDelta* triple) override;
};


//class MergeSnapshotPatchIterator: public TripleDeltaIterator {
//private:
//    TripleDeltaIterator* snapshot_it;
//    PatchTree* patchTree;
//    int patch_id;
//    std::unordered_set<Triple> emitted_additions;
//    std::unordered_set<Triple> emitted_deletions;
//public:
//    MergeSnapshotPatchIterator(TripleDeltaIterator* snapshot_it, PatchTree* patch_tree, int patch_id);
//    ~MergeSnapshotPatchIterator() override;
//    bool next(TripleDelta* triple) override;
//};

#endif //TPFPATCH_STORE_TRIPLEDELTAITERATOR_H
