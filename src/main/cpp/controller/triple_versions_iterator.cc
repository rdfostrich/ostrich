#include "triple_versions_iterator.h"
#include "../snapshot/sorted_triple_iterator.h"
#include <algorithm>
#include <numeric>
#include <utility>



PatchTreeTripleVersionsIterator::PatchTreeTripleVersionsIterator(Triple triple_pattern, hdt::IteratorTripleID* snapshot_it, std::shared_ptr<PatchTree> patchTree, int first_version, std::shared_ptr<DictionaryManager> dictionary)
        : triple_pattern(triple_pattern), snapshot_it(snapshot_it), patchTree(patchTree), addition_it(nullptr), first_version(first_version), dict(dictionary) {}

PatchTreeTripleVersionsIterator::~PatchTreeTripleVersionsIterator() {
    delete snapshot_it;
    delete addition_it;
}

inline void PatchTreeTripleVersionsIterator::eraseDeletedVersions(std::vector<int>* versions, Triple* currentTriple, int initial_version) {
    if (patchTree == nullptr) {
        // If we only have a snapshot, return a single version annotation.
        versions->clear();
        versions->push_back(initial_version);
    } else {
        PatchTreeDeletionValue* deletion = patchTree->get_deletion_value(*currentTriple);
        versions->clear();
        versions->resize(patchTree->get_max_patch_id() + 1 - initial_version);
        std::iota(versions->begin(), versions->end(), initial_version); // Fill up the vector with all versions from initial_version to max_patch_id
        if (deletion != nullptr) {
            for (int v_del = 0; v_del < deletion->get_size(); v_del++) {
                PatchTreeDeletionValueElement deletion_element = deletion->get_patch_at(v_del);
                // Erase-remove idiom on sorted vector, and maintain order
                auto pr = std::equal_range(versions->begin(), versions->end(), deletion_element.get_patch_id());
                versions->erase(pr.first, pr.second);
            }
        }
        delete deletion;
    }
}

bool PatchTreeTripleVersionsIterator::next(TripleVersions* triple_versions) {
    // Loop over snapshot elements, and emit all versions minus the versions that have been deleted.

    triple_versions->set_dictionary(dict);

    if (snapshot_it != nullptr && snapshot_it->hasNext()) {
        hdt::TripleID *tripleId = snapshot_it->next();
        Triple* currentTriple = triple_versions->get_triple();
        currentTriple->set_subject(tripleId->getSubject());
        currentTriple->set_predicate(tripleId->getPredicate());
        currentTriple->set_object(tripleId->getObject());
        eraseDeletedVersions(triple_versions->get_versions(), currentTriple, first_version);
        return true;
    }

    // If we only have a snapshot, don't query the unavailable patch tree.
    if (patchTree == nullptr) {
        return false;
    }

    // When we get here, no snapshot elements are left, so emit additions here
    if (addition_it == nullptr) {
        addition_it = patchTree->addition_iterator(triple_pattern);
    }
#ifdef COMPRESSED_ADD_VALUES
    PatchTreeAdditionValue value(patchTree->get_max_patch_id());
#else
    PatchTreeAdditionValue value;
#endif
    while (addition_it->next_addition(triple_versions->get_triple(), &value)) {
        // Skip if FIRST this addition has a local change for its first patch id,
        // because in that case the triple was originally part of the snapshot, so it's already emitted.
        if (!value.is_local_change(value.get_patch_id_at(0))) {
            eraseDeletedVersions(triple_versions->get_versions(), triple_versions->get_triple(), value.get_patch_id_at(0));
            return true;
        }
    }

    return false;
}


size_t PatchTreeTripleVersionsIterator::get_count() {
    size_t count = 0;
    TripleVersions tv;
    while (next(&tv)) count++;
    return count;
}


PatchTreeTripleVersionsIterator* PatchTreeTripleVersionsIterator::offset(int offset) {
    TripleVersions tv;
    while(offset-- > 0 && next(&tv));
    return this;
}


TripleVersionsIteratorCombined::TripleVersionsIteratorCombined(hdt::TripleComponentOrder order) : comparator(TripleComparator::get_triple_comparator(order)), triples(*comparator) {}

bool TripleVersionsIteratorCombined::next(TripleVersions *triple_versions) {
    if (triples_it == triples.end())
        return false;
    Triple *currentTriple = (*triples_it)->get_triple();
    triple_versions->get_triple()->set_subject(currentTriple->get_subject());
    triple_versions->get_triple()->set_predicate(currentTriple->get_predicate());
    triple_versions->get_triple()->set_object(currentTriple->get_object());
    triple_versions->get_versions()->clear();
    triple_versions->get_versions()->insert(triple_versions->get_versions()->begin(),
                                            (*triples_it)->get_versions()->begin(),
                                            (*triples_it)->get_versions()->end());
    triple_versions->set_dictionary((*triples_it)->get_dictionary());
    triples_it++;
    return true;
}

size_t TripleVersionsIteratorCombined::get_count() {
    return triples.size();
}

TripleVersionsIteratorCombined *TripleVersionsIteratorCombined::offset(int offset) {
    if (offset > triples.size()) {
        triples_it = triples.end();
    } else {
        std::advance(triples_it, offset);
    }
    return this;
}

TripleVersionsIteratorCombined::~TripleVersionsIteratorCombined() {
    delete comparator;
    for (auto t: triples) {
        delete t;
    }
}

void TripleVersionsIteratorCombined::add_iterator(TripleVersionsIterator *it) {
    TripleVersions t;
    while (it->next(&t)) {
        auto pos = triples.find(&t);
        if (pos != triples.end()) {
            std::vector<int> nv;
            nv.reserve(std::max((*pos)->get_versions()->size(), t.get_versions()->size()));
            std::merge(t.get_versions()->begin(), t.get_versions()->end(), (*pos)->get_versions()->begin(), (*pos)->get_versions()->end(), std::back_inserter(nv));
            auto erase_it = std::unique(nv.begin(), nv.end());
            nv.erase(erase_it, nv.end());
            (*pos)->get_versions()->clear();
            (*pos)->get_versions()->insert((*pos)->get_versions()->begin(), nv.begin(), nv.end());
        } else {
            auto* tmp = new Triple;
            tmp->set_subject(t.get_triple()->get_subject());
            tmp->set_predicate(t.get_triple()->get_predicate());
            tmp->set_object(t.get_triple()->get_object());
            auto* v = new std::vector<int>(*(t.get_versions()));
            auto* tv = new TripleVersions(tmp, v, t.get_dictionary());
            triples.insert(tv);
        }
    }
    triples_it = triples.begin();
}


TripleVersionsIteratorMerged::TripleVersionsIteratorMerged(TripleVersionsIterator *iterator1,
                                                           TripleVersionsIterator *iterator2,
                                                           hdt::TripleComponentOrder triple_order): it1(iterator1), it2(iterator2), t1(new TripleVersions), t2(new TripleVersions) {
    status1 = it1->next(t1);
    status2 = it2->next(t2);
    comparator = TripleComparator::get_triple_comparator(triple_order, t1->get_dictionary(), t2->get_dictionary());
}

bool TripleVersionsIteratorMerged::next(TripleVersions *triple_versions) {
    auto emit_triple = [=](const Triple* triple, const std::vector<int>* versions, std::shared_ptr<DictionaryManager> dict) {
        triple_versions->get_triple()->set_subject(triple->get_subject());
        triple_versions->get_triple()->set_predicate(triple->get_predicate());
        triple_versions->get_triple()->set_object(triple->get_object());
        triple_versions->get_versions()->clear();
        triple_versions->get_versions()->insert(triple_versions->get_versions()->begin(), versions->cbegin(), versions->cend());
        triple_versions->set_dictionary(std::move(dict));
    };

    auto merge_versions = [](const std::vector<int>* v1, const std::vector<int>* v2) {
        std::vector<int> v;
        std::merge(v1->cbegin(), v1->cend(), v2->cbegin(), v2->cend(), std::back_inserter(v));
        auto erase_it = std::unique(v.begin(), v.end());
        v.erase(erase_it, v.end());
        return std::move(v);
    };

    if (!status1 && it1 != nullptr) {
        delete it1;
        it1 = nullptr;
    }
    if (!status2 && it2 != nullptr) {
        delete it2;
        it2 = nullptr;
    }

    if (!status1 && !status2) {
        return false;
    }
    if (status1 && !status2) {
        emit_triple(t1->get_triple(), t1->get_versions(), t1->get_dictionary());
        status1 = it1->next(t1);
        return true;
    }
    if (!status1 && status2) {
        emit_triple(t2->get_triple(), t2->get_versions(), t2->get_dictionary());
        status2 = it2->next(t2);
        return true;
    }
    int comp = comparator->compare(*(t1->get_triple()), *(t2->get_triple()));
    if (comp < 0) {
        emit_triple(t1->get_triple(), t1->get_versions(), t1->get_dictionary());
        status1 = it1->next(t1);
        return true;
    }
    if (comp > 0) {
        emit_triple(t2->get_triple(), t2->get_versions(), t2->get_dictionary());
        status2 = it2->next(t2);
        return true;
    }
    std::vector<int> v = merge_versions(t1->get_versions(), t2->get_versions());
    emit_triple(t1->get_triple(), &v, t1->get_dictionary());
    status1 = it1->next(t1);
    status2 = it2->next(t2);
    return true;
}

TripleVersionsIteratorMerged::~TripleVersionsIteratorMerged() {
    delete it1;
    delete it2;
    delete t1;
    delete t2;
    delete comparator;
}

size_t TripleVersionsIteratorMerged::get_count() {
    size_t count = 0;
    TripleVersions tv;
    while (next(&tv)) {
        count++;
    }
    return count;
}

TripleVersionsIteratorMerged *TripleVersionsIteratorMerged::offset(int offset) {
    TripleVersions tv;
    while(offset > 0 && next(&tv)) {
        offset--;
    }
    return this;
}


SortedTripleVersionsIterator::SortedTripleVersionsIterator(TripleVersionsIterator *iterator,
                                                           hdt::TripleComponentOrder order): comparator(TripleComparator::get_triple_comparator(order)), pos(0) {
    TripleVersions tv;
    while(iterator->next(&tv)) {
        auto t = new Triple(tv.get_triple()->get_subject(), tv.get_triple()->get_predicate(), tv.get_triple()->get_object());
        auto v = new std::vector<int>;
        v->reserve(tv.get_versions()->size());
        v->insert(v->begin(), tv.get_versions()->begin(), tv.get_versions()->end());
        triples.push_back(new TripleVersions(t, v, tv.get_dictionary()));
    }
    std::sort(triples.begin(), triples.end(), *comparator);
}

SortedTripleVersionsIterator::~SortedTripleVersionsIterator() {
    for (auto t: triples) {
        delete t;
    }
    delete comparator;
}

bool SortedTripleVersionsIterator::next(TripleVersions *triple_versions) {
    if (pos >= triples.size()) {
        return false;
    }
    TripleVersions* t = triples[pos];
    triple_versions->get_triple()->set_subject(t->get_triple()->get_subject());
    triple_versions->get_triple()->set_predicate(t->get_triple()->get_predicate());
    triple_versions->get_triple()->set_object(t->get_triple()->get_object());
    triple_versions->get_versions()->clear();
    triple_versions->get_versions()->insert(triple_versions->get_versions()->begin(), t->get_versions()->begin(), t->get_versions()->end());
    triple_versions->set_dictionary(t->get_dictionary());
    pos++;
    return true;
}

size_t SortedTripleVersionsIterator::get_count() {
    return triples.size();
}

SortedTripleVersionsIterator *SortedTripleVersionsIterator::offset(int offset) {
    pos += offset;
    return this;
}



void PatchTreeTripleVersionsIteratorV2::eraseDeletedVersions(std::vector<int> *versions, Triple *currentTriple,
                                                             int initial_version) {
    if (patchTree == nullptr) {
        // If we only have a snapshot, return a single version annotation.
        versions->clear();
        versions->push_back(initial_version);
    } else {
        PatchTreeDeletionValue* deletion = patchTree->get_deletion_value(*currentTriple);
        versions->clear();
        versions->resize(patchTree->get_max_patch_id() + 1 - initial_version);
        std::iota(versions->begin(), versions->end(), initial_version); // Fill up the vector with all versions from initial_version to max_patch_id
        if (deletion != nullptr) {
            for (int v_del = 0; v_del < deletion->get_size(); v_del++) {
                PatchTreeDeletionValueElement deletion_element = deletion->get_patch_at(v_del);
                // Erase-remove idiom on sorted vector, and maintain order
                auto pr = std::equal_range(versions->begin(), versions->end(), deletion_element.get_patch_id());
                versions->erase(pr.first, pr.second);
            }
        }
        delete deletion;
    }
}

PatchTreeTripleVersionsIteratorV2::PatchTreeTripleVersionsIteratorV2(Triple triple_pattern, hdt::IteratorTripleID *snapshot_it, std::shared_ptr<PatchTree> patchTree, int first_version, std::shared_ptr<DictionaryManager> dictionary) :
                                                                         triple_pattern(triple_pattern),
                                                                         snapshot_it(nullptr),
                                                                         patchTree(patchTree),
                                                                         addition_it(nullptr),
                                                                         first_version(first_version),
                                                                         dict(dictionary) {

    hdt::TripleComponentOrder qr_order = TripleStore::get_query_order(triple_pattern);
    comparator = std::unique_ptr<TripleComparator>(TripleComparator::get_triple_comparator(qr_order, dict, dict));
    this->snapshot_it = std::unique_ptr<hdt::IteratorTripleID>(snapshot_it);
    if (this->snapshot_it->hasNext()) {
        hdt::TripleID *tripleId = this->snapshot_it->next();
        t1.set_subject(tripleId->getSubject());
        t1.set_predicate(tripleId->getPredicate());
        t1.set_object(tripleId->getObject());
        status1 = true;
    } else {
        status1 = false;
    }

    if (patchTree != nullptr) {
        addition_it = std::unique_ptr<PatchTreeIterator>(patchTree->addition_iterator(triple_pattern));
#ifdef COMPRESSED_ADD_VALUES
        value = std::unique_ptr<PatchTreeAdditionValue>(new PatchTreeAdditionValue(patchTree->get_max_patch_id()));
#else
        value = std::unique_ptr<PatchTreeAdditionValue>(new PatchTreeAdditionValue);
#endif
        status2 = addition_it->next_addition(&t2, value.get());
    } else {
        status2 = false;
    }
}

bool PatchTreeTripleVersionsIteratorV2::next(TripleVersions *triple_versions) {
    auto emit_triple = [] (const Triple& source, Triple& target) {
        target.set_subject(source.get_subject());
        target.set_predicate(source.get_predicate());
        target.set_object(source.get_object());
    };
    auto step_snapshot_it = [=] () {
        if (snapshot_it->hasNext()) {
            hdt::TripleID *tripleId = snapshot_it->next();
            t1.set_subject(tripleId->getSubject());
            t1.set_predicate(tripleId->getPredicate());
            t1.set_object(tripleId->getObject());
            status1 = true;
        } else {
            status1 = false;
        }
    };

    triple_versions->set_dictionary(dict);

    if (status1 && status2) {
        int comp = comparator->compare(t1, t2);
        if (comp == 0) {
            emit_triple(t1, *triple_versions->get_triple());
            eraseDeletedVersions(triple_versions->get_versions(), triple_versions->get_triple(), first_version);
            step_snapshot_it();
            status2 = addition_it->next_addition(&t2, value.get());
        } else if (comp < 0) {
            emit_triple(t1, *triple_versions->get_triple());
            eraseDeletedVersions(triple_versions->get_versions(), triple_versions->get_triple(), first_version);
            step_snapshot_it();
        } else {
            emit_triple(t2, *triple_versions->get_triple());
            eraseDeletedVersions(triple_versions->get_versions(), triple_versions->get_triple(), value->get_patch_id_at(0));
            status2 = addition_it->next_addition(&t2, value.get());
        }
        return true;
    }
    if (status1 && !status2) {
        std::string s1 = t1.to_string(*dict);
        emit_triple(t1, *triple_versions->get_triple());
        eraseDeletedVersions(triple_versions->get_versions(), triple_versions->get_triple(), first_version);
        step_snapshot_it();
        return true;
    }
    if (!status1 && status2) {
        std::string s2 = t2.to_string(*dict);
        emit_triple(t2, *triple_versions->get_triple());
        eraseDeletedVersions(triple_versions->get_versions(), triple_versions->get_triple(), value->get_patch_id_at(0));
        status2 = addition_it->next_addition(&t2, value.get());
        return true;
    }
    return false;
}

size_t PatchTreeTripleVersionsIteratorV2::get_count() {
    size_t count = 0;
    TripleVersions tv;
    while (next(&tv)) {
        count++;
    }
    return count;
}

PatchTreeTripleVersionsIteratorV2 *PatchTreeTripleVersionsIteratorV2::offset(int offset) {
    TripleVersions tv;
    while(offset-- > 0 && next(&tv));
    return this;
}

hdt::TripleComponentOrder PatchTreeTripleVersionsIteratorV2::get_order() {
    return snapshot_it->getOrder();
}


TripleVersionsIteratorCombinedV2::TripleVersionsIteratorCombinedV2(hdt::TripleComponentOrder order) : comparator(TripleComparator::get_triple_comparator(order)) {}

void TripleVersionsIteratorCombinedV2::add_iterator(TripleVersionsIterator *it) {
    auto* t = new TripleVersions;
    bool status = it->next(t);
    auto pos = std::lower_bound(triples.begin(), triples.end(), t, *comparator);
    while (status) {
        if (pos != triples.end()) {
            int comp = comparator->compare(t, (*pos));
            if (comp == 0) {
                std::vector<int> nv;
                std::set_union((*pos)->get_versions()->begin(), (*pos)->get_versions()->end(), t->get_versions()->begin(), t->get_versions()->end(), std::back_inserter(nv));
                (*pos)->get_versions()->clear();
                (*pos)->get_versions()->insert((*pos)->get_versions()->begin(), nv.begin(), nv.end());
                status = it->next(t);
                pos++;
            } else if (comp < 0) {
                pos = triples.insert(pos, t);
                t = new TripleVersions;
                status = it->next(t);
                pos++;
            } else {
                pos = std::lower_bound(pos, triples.end(), t, *comparator);
            }
        } else {
            triples.push_back(t);
            t = new TripleVersions;
            status = it->next(t);
            pos = triples.end();
        }
    }
    triples_it = triples.begin();
    delete t;
}

bool TripleVersionsIteratorCombinedV2::next(TripleVersions *triple_versions) {
    if (triples_it != triples.end()) {
        triple_versions->get_triple()->set_subject((*triples_it)->get_triple()->get_subject());
        triple_versions->get_triple()->set_predicate((*triples_it)->get_triple()->get_predicate());
        triple_versions->get_triple()->set_object((*triples_it)->get_triple()->get_object());
        triple_versions->get_versions()->clear();
        triple_versions->get_versions()->insert(triple_versions->get_versions()->begin(), (*triples_it)->get_versions()->begin(), (*triples_it)->get_versions()->end());
        triple_versions->set_dictionary((*triples_it)->get_dictionary());
        triples_it++;
        return true;
    }
    return false;
}

size_t TripleVersionsIteratorCombinedV2::get_count() {
    return triples.size();
}

TripleVersionsIteratorCombinedV2 *TripleVersionsIteratorCombinedV2::offset(int offset) {
    if (offset > triples.size()) {
        triples_it = triples.end();
    } else {
        std::advance(triples_it, offset);
    }
    return this;
}

TripleVersionsIteratorCombinedV2::~TripleVersionsIteratorCombinedV2() {
    for (auto t: triples) {
        delete t;
    }
}


StandardPatchTreeIteratorAdditionProxy::StandardPatchTreeIteratorAdditionProxy(std::shared_ptr<PatchTree> patch_tree, Triple triple_pattern) {
    addition_it = std::unique_ptr<PatchTreeIterator>(patch_tree->addition_iterator(triple_pattern));
#ifdef COMPRESSED_ADD_VALUES
    value = std::unique_ptr<PatchTreeAdditionValue>(new PatchTreeAdditionValue(patch_tree->get_max_patch_id()));
#else
    value = std::unique_ptr<PatchTreeAdditionValue>(new PatchTreeAdditionValue);
#endif
}

bool StandardPatchTreeIteratorAdditionProxy::next(Triple *triple, int *first_version) {
    bool status = addition_it->next_addition(triple, value.get());
    *first_version = value->get_patch_id_at(0);
    return status;
}


SortedPatchTreeIteratorAdditionProxy::SortedPatchTreeIteratorAdditionProxy(std::shared_ptr<PatchTree> patch_tree,
                                                                           Triple triple_pattern,
                                                                           TripleComparator *comparator) {
    std::unique_ptr<PatchTreeIterator> tmp_it = std::unique_ptr<PatchTreeIterator>(patch_tree->addition_iterator(triple_pattern));
#ifdef COMPRESSED_ADD_VALUES
    std::unique_ptr<PatchTreeAdditionValue> value = std::unique_ptr<PatchTreeAdditionValue>(new PatchTreeAdditionValue(patch_tree->get_max_patch_id()));
#else
    std::unique_ptr<PatchTreeAdditionValue> value = std::unique_ptr<PatchTreeAdditionValue>(new PatchTreeAdditionValue);
#endif

    // Comparison function for sorting the vector
    auto comp = [comparator] (const std::pair<Triple, int>& rhs, const std::pair<Triple, int>& lhs) {
        return comparator->compare(rhs.first, lhs.first) < 0;
    };
    Triple t;
    while (tmp_it->next_addition(&t, value.get())) {
        triples.emplace_back(t, value->get_patch_id_at(0));
    }
    std::sort(triples.begin(), triples.end(), comp);
    triples_it = triples.begin();
}

bool SortedPatchTreeIteratorAdditionProxy::next(Triple *triple, int *first_version) {
    if (triples_it != triples.end()) {
        triple->set_subject((*triples_it).first.get_subject());
        triple->set_predicate((*triples_it).first.get_predicate());
        triple->set_object((*triples_it).first.get_object());
        *first_version = (*triples_it).second;
        triples_it++;
        return true;
    }
    return false;
}
