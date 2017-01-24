#include "patch_element_iterator.h"

PatchElementIterator::PatchElementIterator() {}

PatchElementIterator::~PatchElementIterator() {}

PatchElementIteratorTripleStrings::PatchElementIteratorTripleStrings(DictionaryManager* dict, IteratorTripleString* it, bool additions)
        : dict(dict), it(it), additions(additions) {}

PatchElementIteratorTripleStrings::~PatchElementIteratorTripleStrings() {
    delete it;
}

bool PatchElementIteratorTripleStrings::next(PatchElement* element) {
    if (it->hasNext()) {
        TripleString *tripleString = it->next();
        element->set_triple(
                Triple(tripleString->getSubject(), tripleString->getPredicate(), tripleString->getObject(), dict));
        element->set_addition(additions);
        return true;
    }
    return false;
}

void PatchElementIteratorTripleStrings::goToStart() {
    it->goToStart();
}

PatchElementIteratorCombined::PatchElementIteratorCombined() : pos(0), iterators(), passed(0) {}

PatchElementIteratorCombined::~PatchElementIteratorCombined() {
    for (int i = 0; i < iterators.size(); i++) {
        delete iterators[i];
    }
}

bool PatchElementIteratorCombined::next(PatchElement *element) {
    while (!(iterators[pos]->next(element) && ++passed)) {
        pos++;
    }
    return pos < iterators.size();
}

void PatchElementIteratorCombined::appendIterator(PatchElementIterator *it) {
    iterators.push_back(it);
}

long PatchElementIteratorCombined::getPassed() {
    return passed;
}

void PatchElementIteratorCombined::goToStart() {
    while (pos >= 0) {
        if (pos < iterators.size()) {
            iterators[pos]->goToStart();
        }
        pos--;
    }
    pos++;
}

PatchElementIteratorVector::PatchElementIteratorVector(const std::vector<PatchElement>* elements) : elements(elements) {
    goToStart();
}

bool PatchElementIteratorVector::next(PatchElement* element) {
    if (it != elements->end()) {
        element->set_triple(it->get_triple());
        element->set_addition(it->is_addition());
        it++;
        return true;
    }
    return false;
}

void PatchElementIteratorVector::goToStart() {
    it = elements->begin();
}
