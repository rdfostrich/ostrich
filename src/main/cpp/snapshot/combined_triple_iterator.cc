#include <SingleTriple.hpp>
#include "vector_triple_iterator.h"

using namespace hdt;

VectorTripleIterator::VectorTripleIterator(std::vector<TripleString> triples) : triples(triples), pos(-1) {}

bool VectorTripleIterator::hasNext() {
    return pos + 1 < triples.size();
}

TripleString* VectorTripleIterator::next() {
    return &triples[++pos];
}

bool VectorTripleIterator::hasPrevious() {
    return pos >= 0;
}

TripleString* VectorTripleIterator::previous() {
    return &triples[pos--];
}

void VectorTripleIterator::goToStart() {
    pos = -1;
}
