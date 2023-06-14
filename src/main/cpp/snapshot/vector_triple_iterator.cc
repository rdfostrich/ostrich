#include <SingleTriple.hpp>
#include "vector_triple_iterator.h"


VectorTripleIterator::VectorTripleIterator(std::vector<hdt::TripleString> triples) : triples(triples), pos(-1) {}

bool VectorTripleIterator::hasNext() {
    return pos + 1 < triples.size();
}

hdt::TripleString* VectorTripleIterator::next() {
    return &triples[++pos];
}

bool VectorTripleIterator::hasPrevious() {
    return pos >= 0;
}

hdt::TripleString* VectorTripleIterator::previous() {
    return &triples[pos--];
}

void VectorTripleIterator::goToStart() {
    pos = -1;
}
