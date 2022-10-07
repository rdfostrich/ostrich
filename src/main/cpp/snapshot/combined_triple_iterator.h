#ifndef TPFPATCH_STORE_COMBINED_TRIPLE_ITERATOR_H
#define TPFPATCH_STORE_COMBINED_TRIPLE_ITERATOR_H

#include <vector>
#include "../patch/triple.h"
#include <Triples.hpp>


class CombinedTripleIterator : public hdt::IteratorTripleString {
private:
    int pos;
    std::vector<IteratorTripleString*> iterators;
public:
    CombinedTripleIterator();
    ~CombinedTripleIterator();
    bool hasNext();
    hdt::TripleString *next();
    void appendIterator(IteratorTripleString* it);
    void goToStart();
};


#endif //TPFPATCH_STORE_COMBINED_TRIPLE_ITERATOR_H
