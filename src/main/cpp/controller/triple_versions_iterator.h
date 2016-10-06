#ifndef TPFPATCH_STORE_TRIPLEVERSIONITERATOR_H
#define TPFPATCH_STORE_TRIPLEVERSIONITERATOR_H

#include <vector>
#include "../patch/triple.h"
#include "../patch/patch_tree.h"

// Triple annotated with addition/deletion.
class TripleVersions {
protected:
    Triple* triple;
    std::vector<int>* versions;
public:
    TripleVersions();
    ~TripleVersions();
    Triple* get_triple();
    std::vector<int>* get_versions();
};

class TripleVersionsIterator {
protected:
    Triple triple_pattern;
    IteratorTripleID* snapshot_it;
    PatchTree* patchTree;
    PatchTreeIterator* addition_it;
    inline void eraseDeletedVersions(std::vector<int>* versions, Triple* currentTriple, int initial_version);
public:
    TripleVersionsIterator(Triple triple_pattern, IteratorTripleID* snapshot_it, PatchTree* patchTree);
    bool next(TripleVersions* triple_versions);
    size_t get_count();
    TripleVersionsIterator* offset(int offset);
};


#endif //TPFPATCH_STORE_TRIPLEVERSIONITERATOR_H
