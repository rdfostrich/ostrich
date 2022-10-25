#ifndef TPFPATCH_STORE_COMBINED_TRIPLE_ITERATOR_H
#define TPFPATCH_STORE_COMBINED_TRIPLE_ITERATOR_H

#include <vector>
#include "../patch/triple.h"
#include <Triples.hpp>


class CombinedTripleIterator : public hdt::IteratorTripleString {
private:
    int pos;
    std::vector<hdt::IteratorTripleString*> iterators;
public:
    CombinedTripleIterator();
    ~CombinedTripleIterator() override;
    bool hasNext() override;
    hdt::TripleString *next() override;
    void appendIterator(hdt::IteratorTripleString* it);
    void goToStart() override;
};


#endif //TPFPATCH_STORE_COMBINED_TRIPLE_ITERATOR_H
