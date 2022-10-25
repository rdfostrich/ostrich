#ifndef TPFPATCH_STORE_SNAPSHOT_ITERATOR_TRIPLE_STRING_H
#define TPFPATCH_STORE_SNAPSHOT_ITERATOR_TRIPLE_STRING_H

#include <Triples.hpp>
#include <Iterator.hpp>
#include <HDT.hpp>
#include "../patch/positioned_triple_iterator.h"
#include "../patch/patch_tree.h"

class SnapshotPatchIteratorTripleID : public TripleIterator {
private:
    hdt::IteratorTripleID* snapshot_it;
    PositionedTripleIterator* deletion_it;
    PatchTreeTripleIterator* addition_it;
    PatchTreeKeyComparator* spo_comparator;

    bool has_last_deleted_triple;
    PositionedTriple* last_deleted_triple;
    std::shared_ptr<hdt::HDT> snapshot;
    const Triple triple_pattern;
    std::shared_ptr<PatchTree> patchTree;
    int patch_id;
    int offset;
    PatchPosition deletion_count;
    std::shared_ptr<DictionaryManager> dict;
public:
    SnapshotPatchIteratorTripleID(hdt::IteratorTripleID* snapshot_it, PositionedTripleIterator* deletion_it,
                                  PatchTreeKeyComparator* spo_comparator, std::shared_ptr<hdt::HDT> snapshot, const Triple& triple_pattern,
                                  std::shared_ptr<PatchTree> patchTree, int patch_id, int offset, PatchPosition deletion_count, std::shared_ptr<DictionaryManager> dict);
    ~SnapshotPatchIteratorTripleID();
    bool next(Triple* triple);
};


#endif //TPFPATCH_STORE_SNAPSHOT_ITERATOR_TRIPLE_STRING_H
