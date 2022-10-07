#ifndef TPFPATCH_STORE_TRIPLEVERSIONITERATOR_H
#define TPFPATCH_STORE_TRIPLEVERSIONITERATOR_H

#include <vector>
#include <set>
#include "../patch/triple.h"
#include "../patch/patch_tree.h"
#include "../patch/triple_comparator.h"


class TripleVersionsIterator {
public:
    virtual bool next(TripleVersions* triple_versions) = 0;
    virtual size_t get_count() = 0;
    virtual TripleVersionsIterator* offset(int offset) = 0;
    virtual ~TripleVersionsIterator() = default;
};


class PatchTreeTripleVersionsIterator: public TripleVersionsIterator {
protected:
    Triple triple_pattern;
    hdt::IteratorTripleID* snapshot_it;
    std::shared_ptr<PatchTree> patchTree;
    PatchTreeIterator* addition_it;
    int first_version;
    inline void eraseDeletedVersions(std::vector<int>* versions, Triple* currentTriple, int initial_version);
    std::shared_ptr<DictionaryManager> dict;
public:
    PatchTreeTripleVersionsIterator(Triple triple_pattern, hdt::IteratorTripleID* snapshot_it, std::shared_ptr<PatchTree> patchTree, int first_version = 0, std::shared_ptr<DictionaryManager> dictionary = nullptr);
    ~PatchTreeTripleVersionsIterator() override;
    bool next(TripleVersions* triple_versions) override;
    size_t get_count() override;
    PatchTreeTripleVersionsIterator* offset(int offset) override;
};


class TripleVersionsIteratorCombined: public TripleVersionsIterator {
private:
    TripleComparator* comparator;
    std::set<TripleVersions*, TripleComparator> triples;
    std::set<TripleVersions*, TripleComparator>::iterator triples_it;

public:
    explicit TripleVersionsIteratorCombined(hdt::TripleComponentOrder order);
    ~TripleVersionsIteratorCombined() override;
    /**
     * Add an iterator
     * reset the internal position of the TripleVersionsIteratorCombined
     * consume all triples in the iterator
     * @param it the iterator to add
     */
    void add_iterator(TripleVersionsIterator* it);
    bool next(TripleVersions* triple_versions) override;
    size_t get_count() override;
    TripleVersionsIteratorCombined* offset(int offset) override;
};


class TripleVersionsIteratorMerged: public TripleVersionsIterator {
private:
    TripleVersionsIterator* it1;
    TripleVersionsIterator* it2;
    TripleVersions* t1;
    TripleVersions* t2;
    bool status1;
    bool status2;
    TripleComparator* comparator;

public:
    TripleVersionsIteratorMerged(TripleVersionsIterator* iterator1, TripleVersionsIterator* iterator2, hdt::TripleComponentOrder triple_order);
    ~TripleVersionsIteratorMerged() override;
    bool next(TripleVersions* triple_versions) override;
    size_t get_count() override;
    TripleVersionsIteratorMerged* offset(int offset) override;
};


class SortedTripleVersionsIterator: public TripleVersionsIterator {
private:
    TripleComparator* comparator;
    std::vector<TripleVersions*> triples;
    size_t pos;
public:
    SortedTripleVersionsIterator(TripleVersionsIterator* iterator, hdt::TripleComponentOrder order);
    ~SortedTripleVersionsIterator() override;
    bool next(TripleVersions* triple_versions) override;
    size_t get_count() override;
    SortedTripleVersionsIterator* offset(int offset) override;
};

#endif //TPFPATCH_STORE_TRIPLEVERSIONITERATOR_H
