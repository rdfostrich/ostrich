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
        if (++pos == iterators.size()) break;
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
    thread = std::thread(std::bind(&PatchElementIteratorBuffered::fill_buffer, this));
    buffer_trigger_fill.notify_all();
}

bool PatchElementIteratorBuffered::next(PatchElement* element) {
    // Wait for fill-buffer thread if the buffer is empty at the moment
    {
        std::unique_lock<std::mutex> l(lock_thread_nonempty);
        while (!ended && buffer.size() < 1) {
            buffer_trigger_nonempty.wait(l);
        }
    }
    if (!ended || buffer.size() > 0) {
        // Get first element from buffer
        PatchElement& buffer_element = buffer.front();
        buffer.pop();
        // If the inner iterator hasn't ended yet, and our buffer is half-empty, notify the fill-buffer thread.
        if (!ended && buffer.size() < buffer_size / 2) {
            buffer_trigger_fill.notify_all();
        }
        element->set_triple(buffer_element.get_triple());
        element->set_addition(buffer_element.is_addition());
        return true;
    }
    return false;
}

void PatchElementIteratorBuffered::goToStart() {
    std::queue<PatchElement> empty;
    std::swap(buffer, empty);
    it->goToStart();
    buffer_trigger_fill.notify_all();
    // This may have problems when calling this method _before_ the inner iterator has ended,
    // but let's ignore this for now since it's not applicable...
}

void PatchElementIteratorBuffered::fill_buffer() {
    PatchElement element;
    while (!shutdown_thread) {
        while (buffer.size() < buffer_size) {
            // Get inner iterator element, and add it to the buffer.
            if (it->next(&element)) {
                lock_thread_nonempty.lock();
                buffer.emplace(element.get_triple(), element.is_addition());
                lock_thread_nonempty.unlock();
                // Notify the other thread to tell that the buffer is not empty.
                buffer_trigger_nonempty.notify_all();
            } else {
                lock_thread_nonempty.lock();
                ended = true;
                lock_thread_nonempty.unlock();
                // Notify the other thread to tell that the iterator has ended.
                buffer_trigger_nonempty.notify_all();
                break;
            }
        }
        if (shutdown_thread) break;
        // Wait until a request for further filling the buffer.
        {
            std::unique_lock<std::mutex> l(lock_thread_fill);
            buffer_trigger_fill.wait(l);
        }
    }
}

PatchElementIteratorBuffered::~PatchElementIteratorBuffered() {
    shutdown_thread = true;
    buffer_trigger_fill.notify_all(); // Because the fill-buffer thread could still be waiting! (avoids deadlock)
    thread.join();
}
