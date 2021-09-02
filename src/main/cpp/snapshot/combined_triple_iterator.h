#ifndef TPFPATCH_STORE_COMBINED_TRIPLE_ITERATOR_H
#define TPFPATCH_STORE_COMBINED_TRIPLE_ITERATOR_H

#include <vector>
#include "../patch/triple.h"
#include <Triples.hpp>

using namespace hdt;

class CombinedTripleIterator : public IteratorTripleString {
private:
    int pos;
    std::vector<IteratorTripleString*> iterators;
public:
    CombinedTripleIterator();
    ~CombinedTripleIterator();
    bool hasNext();
    TripleString *next();
    void appendIterator(IteratorTripleString* it);
    void goToStart();
};


#endif //TPFPATCH_STORE_COMBINED_TRIPLE_ITERATOR_H
