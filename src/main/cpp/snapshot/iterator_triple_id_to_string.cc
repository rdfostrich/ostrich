#include "iterator_triple_id_to_string.h"

// @deprecated TODO: remove
IteratorTripleIdToString::IteratorTripleIdToString(HDT *hdt, TripleID tripleId) {
    it = hdt->getTriples()->search(tripleId);
}

IteratorTripleIdToString::~IteratorTripleIdToString() {
    delete it;
}

TripleString* IteratorTripleIdToString::toTripleString(TripleID* tripleId) {
    TripleString* tripleString = new TripleString();
    dict->tripleIDtoTripleString(*tripleId, *tripleString);
    return tripleString;
}

bool IteratorTripleIdToString::hasNext() {
    return it->hasNext();
}

TripleString* IteratorTripleIdToString::next() {
    return toTripleString(it->next());
}

bool IteratorTripleIdToString::hasPrevious() {
    return it->hasPrevious();
}

TripleString* IteratorTripleIdToString::previous() {
    return toTripleString(it->previous());
}

bool IteratorTripleIdToString::canGoTo() {
    return true;
}

void IteratorTripleIdToString::goToStart() {
    it->goToStart();
}

void IteratorTripleIdToString::goTo(unsigned int pos) {
    if(it->canGoTo()) {
        try {
            it->goTo(pos);
            pos = 0;
        } catch (char const* error) {}
    }
    while(pos-- > 0 && it->hasNext()) it->next();
}

IteratorTripleID *IteratorTripleIdToString::getTripleIdIterator() {
    return it;
}
