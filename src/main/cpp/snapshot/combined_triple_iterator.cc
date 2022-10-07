#include <SingleTriple.hpp>
#include "combined_triple_iterator.h"

CombinedTripleIterator::CombinedTripleIterator() : iterators() {}

bool CombinedTripleIterator::hasNext() {
    /*cout << iterators[0]->hasNext() << endl;
    iterators[pos]->hasPrevious();
    cout << iterators[0]->hasNext() << endl;*/
    while (pos < iterators.size() && !iterators[pos]->hasNext()) {
        pos++;
    }
    //cout << "pos: " << pos << endl;
    //cout << "check: " << (pos < iterators.size() && iterators[pos]->hasNext()) << endl;

    return pos < iterators.size();
}

hdt::TripleString* CombinedTripleIterator::next() {
    return iterators[pos]->next();
}

void CombinedTripleIterator::appendIterator(IteratorTripleString* it) {
    iterators.push_back(it);
}

void CombinedTripleIterator::goToStart() {
    while (pos >= 0) {
        if (pos < iterators.size()) {
            iterators[pos]->goToStart();
        }
        pos--;
    }
    pos++;
}

CombinedTripleIterator::~CombinedTripleIterator() {
    for (auto it: iterators)
        delete it;
}
