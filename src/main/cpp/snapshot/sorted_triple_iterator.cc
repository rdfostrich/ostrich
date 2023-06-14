#include "sorted_triple_iterator.h"


SortedTripleIterator::SortedTripleIterator(hdt::IteratorTripleID *source_it, hdt::TripleComponentOrder order, std::shared_ptr<DictionaryManager> dict) : order(order) {
    while (source_it->hasNext()) {
        hdt::TripleID* tmp_t = source_it->next();
        triples.emplace_back(tmp_t->getSubject(), tmp_t->getPredicate(), tmp_t->getObject());
    }
    std::unique_ptr<TripleComparator> comparator = std::unique_ptr<TripleComparator>(TripleComparator::get_triple_comparator(order, dict, dict));
    std::sort(triples.begin(), triples.end(), *comparator);
    pos = triples.begin();
    delete source_it;
}

bool SortedTripleIterator::hasNext() {
    return pos != triples.end();
}

hdt::TripleID *SortedTripleIterator::next() {
    hdt::TripleID* ret_t = &(*pos);
    pos++;
    return ret_t;
}

bool SortedTripleIterator::hasPrevious() {
    return pos != triples.begin();
}

hdt::TripleID *SortedTripleIterator::previous() {
    pos--;
    hdt::TripleID* ret_t = &(*pos);
    return ret_t;
}

void SortedTripleIterator::goToStart() {
    pos = triples.begin();
}

size_t SortedTripleIterator::estimatedNumResults() {
    return triples.size();
}

hdt::ResultEstimationType SortedTripleIterator::numResultEstimation() {
    return hdt::EXACT;
}

bool SortedTripleIterator::canGoTo() {
    return true;
}

void SortedTripleIterator::goTo(size_t pos) {
    this->pos = triples.begin();
    std::advance(this->pos, pos);
}

void SortedTripleIterator::skip(size_t pos) {
    std::advance(this->pos, pos);
}

bool SortedTripleIterator::findNextOccurrence(size_t value, unsigned char component) {
    return false;
}

hdt::TripleComponentOrder SortedTripleIterator::getOrder() {
    return order;
}

bool SortedTripleIterator::isSorted(hdt::TripleComponentRole role) {
    return true;
}
