#ifndef TPFPATCH_STORE_PATCH_ELEMENT_ITERATOR_H
#define TPFPATCH_STORE_PATCH_ELEMENT_ITERATOR_H

#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include "patch_element.h"

class PatchElementIterator {
public:
    PatchElementIterator();
    virtual ~PatchElementIterator() = 0;
    virtual bool next(PatchElement* element) = 0;
    virtual void goToStart() = 0;
    virtual size_t getPassed() = 0;
};

class PatchElementIteratorTripleStrings : public PatchElementIterator {
protected:
    std::shared_ptr<DictionaryManager> dict;
    hdt::IteratorTripleString* it;
    bool additions;
    size_t passed;
public:
    PatchElementIteratorTripleStrings(std::shared_ptr<DictionaryManager> dict, hdt::IteratorTripleString* subIt, bool additions);
    ~PatchElementIteratorTripleStrings() override;
    bool next(PatchElement* element) override;
    void goToStart() override;
    size_t getPassed() override;
};

class PatchElementIteratorCombined : public PatchElementIterator {
protected:
    std::vector<PatchElementIterator*> iterators;
    size_t passed;
    PatchTreeKeyComparator comparator;
    std::vector<bool> iterators_buffer_valid;
    std::vector<PatchElement*> iterators_buffer;
public:
    explicit PatchElementIteratorCombined(PatchTreeKeyComparator comparator);
    ~PatchElementIteratorCombined() override;
    bool next(PatchElement* element) override;
    void appendIterator(PatchElementIterator* it);
    size_t getPassed() override;
    void goToStart() override;
};

class PatchElementIteratorVector : public PatchElementIterator {
protected:
    const std::vector<PatchElement>* elements;
    std::vector<PatchElement>::const_iterator it;
    size_t passed;
public:
    explicit PatchElementIteratorVector(const std::vector<PatchElement>* elements);
    bool next(PatchElement* element) override;
    void goToStart() override;
    size_t getPassed() override;
};

class PatchElementIteratorBuffered : public PatchElementIterator {
protected:
    PatchElementIterator* it;
    std::queue<PatchElement> buffer;
    unsigned long buffer_size;
    bool ended;
    std::condition_variable buffer_trigger_fill;
    std::condition_variable buffer_trigger_nonempty;
    bool shutdown_thread = false;
    std::mutex lock_thread_fill;
    std::mutex lock_thread_nonempty;
    std::thread thread;
    std::atomic<size_t> passed;
protected:
    void fill_buffer();
public:
    PatchElementIteratorBuffered(PatchElementIterator* it, unsigned long buffer_size);
    ~PatchElementIteratorBuffered() override;
    bool next(PatchElement* element) override;
    void goToStart() override;
    size_t getPassed() override;
};

class IteratorTripleStringVector : public hdt::IteratorTripleString {
protected:
    const std::vector<hdt::TripleString>* elements;
    std::vector<hdt::TripleString>::const_iterator it;
public:
    explicit IteratorTripleStringVector(const std::vector<hdt::TripleString>* elements);
    bool hasNext() override;
    hdt::TripleString *next() override;
    void goToStart() override;
};

#endif //TPFPATCH_STORE_PATCH_ELEMENT_ITERATOR_H
