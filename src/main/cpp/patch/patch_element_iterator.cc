#include "patch_element_iterator.h"

PatchElementIterator::PatchElementIterator() = default;

PatchElementIterator::~PatchElementIterator() = default;

PatchElementIteratorTripleStrings::PatchElementIteratorTripleStrings(std::shared_ptr<DictionaryManager> dict, hdt::IteratorTripleString* it, bool additions)
        : dict(dict), it(it), additions(additions), passed(0) {}

PatchElementIteratorTripleStrings::~PatchElementIteratorTripleStrings() {
    delete it;
}

bool PatchElementIteratorTripleStrings::next(PatchElement* element) {
    if (it->hasNext()) {
        hdt::TripleString* tripleString = it->next();
        element->set_triple(
                Triple(tripleString->getSubject(), tripleString->getPredicate(), tripleString->getObject(), dict));
        element->set_addition(additions);
        passed++;
        return true;
    }
    return false;
}

void PatchElementIteratorTripleStrings::goToStart() {
    it->goToStart();
}

size_t PatchElementIteratorTripleStrings::getPassed() {
    return passed;
}

PatchElementIteratorCombined::PatchElementIteratorCombined(PatchTreeKeyComparator comparator) : iterators(), passed(0), comparator(comparator) {}

PatchElementIteratorCombined::~PatchElementIteratorCombined() {
    for (int i = 0; i < iterators.size(); i++) {
        delete iterators[i];
        delete iterators_buffer[i];
    }
}

bool PatchElementIteratorCombined::next(PatchElement* element) {
    int chosen_it = -1;
    for (int i = 0; i < iterators.size(); i++) {
        if (!iterators_buffer_valid[i]) {
            iterators_buffer_valid[i] = iterators[i]->next(iterators_buffer[i]);
            passed++;
        }

        if (iterators_buffer_valid[i]) {
            if (chosen_it < 0 || comparator.compare(iterators_buffer[i]->get_triple(), iterators_buffer[chosen_it]->get_triple()) < 0) {
                chosen_it = i;
            }
        }
    }
    if (chosen_it >= 0) {
        iterators_buffer_valid[chosen_it] = false;
        std::swap(*element, *(iterators_buffer[chosen_it]));
        return true;
    }
    return false;
}

void PatchElementIteratorCombined::appendIterator(PatchElementIterator *it) {
    iterators.push_back(it);
    iterators_buffer.push_back(new PatchElement());
    iterators_buffer_valid.push_back(false);
}

size_t PatchElementIteratorCombined::getPassed() {
    return passed;
}

void PatchElementIteratorCombined::goToStart() {
    for (int i = 0; i < iterators.size(); i++) {
        iterators[i]->goToStart();
    }
}

PatchElementIteratorVector::PatchElementIteratorVector(const std::vector<PatchElement>* elements) : elements(elements), passed(0) {
    goToStart();
}

bool PatchElementIteratorVector::next(PatchElement* element) {
    if (it != elements->end()) {
        element->set_triple(it->get_triple());
        element->set_addition(it->is_addition());
        it++;
        passed++;
        return true;
    }
    return false;
}

void PatchElementIteratorVector::goToStart() {
    it = elements->begin();
}

size_t PatchElementIteratorVector::getPassed() {
    return passed;
}

PatchElementIteratorBuffered::PatchElementIteratorBuffered(PatchElementIterator* it, unsigned long buffer_size)
        : it(it), buffer_size(buffer_size), ended(false), passed(0) {
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
        PatchElement buffer_element = buffer.front();
        element->set_triple(buffer_element.get_triple());
        element->set_addition(buffer_element.is_addition());
        buffer.pop();
        // If the inner iterator hasn't ended yet, and our buffer is half-empty, notify the fill-buffer thread.
        if (!ended && buffer.size() < buffer_size / 2) {
            buffer_trigger_fill.notify_all();
        }
        passed++;
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
                buffer.emplace(Triple(element.get_triple()), element.is_addition());
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

size_t PatchElementIteratorBuffered::getPassed() {
    return passed;
}

IteratorTripleStringVector::IteratorTripleStringVector(const std::vector<hdt::TripleString>* elements) : elements(elements) {
    goToStart();
}

bool IteratorTripleStringVector::hasNext() {
    return it != elements->end();
}

hdt::TripleString* IteratorTripleStringVector::next() {
    return (hdt::TripleString*) &(*it++);
}

void IteratorTripleStringVector::goToStart() {
    it = elements->begin();
}
