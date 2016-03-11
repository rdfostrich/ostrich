#ifndef TPFPATCH_STORE_ITERATOR_TRIPLE_ID_TO_STRING_H
#define TPFPATCH_STORE_ITERATOR_TRIPLE_ID_TO_STRING_H

#include <hdt/BasicHDT.hpp>

using namespace hdt;

// A wrapper that contains an IteratorTripleID and exposes it as an IteratorTripleString
class IteratorTripleIdToString : public IteratorTripleString {
private:
    Dictionary* dict;
    IteratorTripleID* it;
protected:
    TripleString* toTripleString(TripleID* tripleId);
public:
    IteratorTripleIdToString(HDT *hdt, TripleString tripleString);
    ~IteratorTripleIdToString();
    bool hasNext();
    TripleString* next();
    bool hasPrevious();
    TripleString* previous();
    bool canGoTo();
    void goToStart();
    void goTo(unsigned int pos);
};


#endif //TPFPATCH_STORE_ITERATOR_TRIPLE_ID_TO_STRING_H
