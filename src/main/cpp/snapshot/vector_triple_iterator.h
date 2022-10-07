#ifndef TPFPATCH_STORE_VECTOR_TRIPLE_ITERATOR_H
#define TPFPATCH_STORE_VECTOR_TRIPLE_ITERATOR_H

#include <vector>
#include "../patch/triple.h"
#include <Triples.hpp>

class VectorTripleIterator : public hdt::IteratorTripleString {
private:
    int pos;
    std::vector<hdt::TripleString> triples;
public:
    VectorTripleIterator(std::vector<hdt::TripleString> triples);
    bool hasNext();
    hdt::TripleString *next();
    bool hasPrevious();
    hdt::TripleString *previous();
    void goToStart();

};


#endif //TPFPATCH_STORE_VECTOR_TRIPLE_ITERATOR_H
