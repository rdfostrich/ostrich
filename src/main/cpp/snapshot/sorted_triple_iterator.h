#ifndef OSTRICH_SORTED_TRIPLE_ITERATOR_H
#define OSTRICH_SORTED_TRIPLE_ITERATOR_H

#include "../patch/triple_comparator.h"
#include <Triples.hpp>


class SortedTripleIterator: public hdt::IteratorTripleID {
private:
    std::vector<hdt::TripleID>::iterator pos;
    std::vector<hdt::TripleID> triples;

    hdt::TripleComponentOrder order;

public:
    SortedTripleIterator(hdt::IteratorTripleID* source_it, hdt::TripleComponentOrder order, std::shared_ptr<DictionaryManager> dict);

    bool hasNext() override;
    hdt::TripleID* next() override;
    bool hasPrevious() override;
    hdt::TripleID* previous() override;
    void goToStart() override;
    size_t estimatedNumResults() override;
    hdt::ResultEstimationType numResultEstimation() override;
    bool canGoTo() override;
    void goTo(size_t pos) override;
    void skip(size_t pos) override;
    bool findNextOccurrence(size_t value, unsigned char component) override;
    hdt::TripleComponentOrder getOrder() override;
    bool isSorted(hdt::TripleComponentRole role) override;
};


#endif //OSTRICH_SORTED_TRIPLE_ITERATOR_H
