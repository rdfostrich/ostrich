#ifndef TPFPATCH_STORE_VECTOR_TRIPLE_ITERATOR_H
#define TPFPATCH_STORE_VECTOR_TRIPLE_ITERATOR_H

#include <vector>
#include "../patch/triple.h"
#include <Triples.hpp>

using namespace hdt;

class VectorTripleIterator : public IteratorTripleString {
private:
    int pos;
    std::vector<TripleString> triples;
public:
    VectorTripleIterator(std::vector<TripleString> triples);
    bool hasNext();
    TripleString *next();
    bool hasPrevious();
    TripleString *previous();
    void goToStart();

};


#endif //TPFPATCH_STORE_VECTOR_TRIPLE_ITERATOR_H
