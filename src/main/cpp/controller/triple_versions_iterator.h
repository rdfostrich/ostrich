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
    TripleVersions(Triple* triple, std::vector<int>* versions);
    ~TripleVersions();
    Triple* get_triple();
    std::vector<int>* get_versions();
};

class TripleVersionsString {
protected:
    StringTriple triple;
    std::vector<int> versions;
public:
    TripleVersionsString();
    TripleVersionsString(StringTriple triple, std::vector<int> versions);
    StringTriple* get_triple();
    std::vector<int>* get_versions();

    // Compare triples strings, not versions, so we can sort a vector of TripleVersionsString
    bool operator<(const TripleVersionsString& other) const;
};


class TripleVersionsIterator {
protected:
    Triple triple_pattern;
    IteratorTripleID* snapshot_it;
    std::shared_ptr<PatchTree> patchTree;
    PatchTreeIterator* addition_it;
    int first_version;
    inline void eraseDeletedVersions(std::vector<int>* versions, Triple* currentTriple, int initial_version);
public:
    TripleVersionsIterator(Triple triple_pattern, IteratorTripleID* snapshot_it, std::shared_ptr<PatchTree> patchTree, int first_version = 0);
    ~TripleVersionsIterator();
    bool next(TripleVersions* triple_versions);
    size_t get_count();
    TripleVersionsIterator* offset(int offset);
};



class TripleVersionsIteratorCombined {
private:
    size_t index;
    std::vector<TripleVersionsString> triples;
public:
    TripleVersionsIteratorCombined();
    void append_iterator(TripleVersionsIterator* iterator, std::shared_ptr<DictionaryManager> dict);
    bool next(TripleVersionsString* triple_versions);
    size_t get_count();
    TripleVersionsIteratorCombined* offset(int offset);
};

#endif //TPFPATCH_STORE_TRIPLEVERSIONITERATOR_H
