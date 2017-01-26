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
        TripleString* tripleString = it->next();
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

PatchElementIteratorBuffered::PatchElementIteratorBuffered(PatchElementIterator* it, unsigned long buffer_size)
        : it(it), buffer_size(buffer_size), ended(false) {
    fill_buffer();
}

bool PatchElementIteratorBuffered::next(PatchElement* element) {
    if (buffer.size() > 0) {
        PatchElement& buffer_element = buffer.front();
        element->set_triple(buffer_element.get_triple());
        element->set_addition(buffer_element.is_addition());
        buffer.pop();
        if (!ended && buffer.size() == 0) { // TODO: if multithreading buffer.size() < buffer_size / 2
            fill_buffer();
        }
        return true;
    }
    return false;
}

void PatchElementIteratorBuffered::goToStart() {
    std::queue<PatchElement> empty;
    std::swap(buffer, empty);
}

void PatchElementIteratorBuffered::fill_buffer() {
    // TODO: different thread?
    for (unsigned long to_fill = buffer_size - buffer.size(); to_fill > 0; to_fill--) {
        PatchElement element(Triple(0, 0, 0), false);
        if (it->next(&element)) {
            buffer.push(element);
        } else {
            ended = true;
            break;
        }
    }
}

PatchElementIteratorBuffered::~PatchElementIteratorBuffered() {
    delete it;
}
