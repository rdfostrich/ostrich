#include "triple_versions_iterator.h"
#include <algorithm>
#include <numeric>
#include <utility>

TripleVersions::TripleVersions() : triple(new Triple()), versions(new vector<int>()) {}

TripleVersions::TripleVersions(Triple* triple, std::vector<int>* versions) : triple(triple), versions(versions) {}

TripleVersions::~TripleVersions() {
    delete triple;
    delete versions;
}


Triple* TripleVersions::get_triple() {
    return triple;
}


vector<int>* TripleVersions::get_versions() {
    return versions;
}

TripleVersionsIterator::TripleVersionsIterator(Triple triple_pattern, IteratorTripleID* snapshot_it, PatchTree* patchTree, int first_version)
        : triple_pattern(triple_pattern), snapshot_it(snapshot_it), patchTree(patchTree), addition_it(NULL), first_version(first_version) {}

TripleVersionsIterator::~TripleVersionsIterator() {
    if (snapshot_it != NULL) delete snapshot_it;
    if (addition_it != NULL) delete addition_it;
}

inline void TripleVersionsIterator::eraseDeletedVersions(std::vector<int>* versions, Triple* currentTriple, int initial_version) {
    if (patchTree == NULL) {
        // If we only have a snapshot, return a single version annotation.
        versions->clear();
        versions->push_back(initial_version);
    } else {
        PatchTreeDeletionValue* deletion = patchTree->get_deletion_value(*currentTriple);
        versions->clear();
        versions->resize(patchTree->get_max_patch_id() + 1 - initial_version);
        std::iota(versions->begin(), versions->end(), initial_version); // Fill up the vector with all versions from initial_version to max_patch_id
        if (deletion != NULL) {
            for (int v_del = 0; v_del < deletion->get_size(); v_del++) {
                PatchTreeDeletionValueElement deletion_element = deletion->get_patch(v_del);
                // Erase-remove idiom on sorted vector, and maintain order
                auto pr = std::equal_range(versions->begin(), versions->end(), deletion_element.get_patch_id());
                versions->erase(pr.first, pr.second);
            }
        }
    }
}

bool TripleVersionsIterator::next(TripleVersions* triple_versions) {
    // Loop over snapshot elements, and emit all versions minus the versions that have been deleted.

    if (snapshot_it != NULL && snapshot_it->hasNext()) {
        TripleID *tripleId = snapshot_it->next();
        Triple* currentTriple = triple_versions->get_triple();
        currentTriple->set_subject(tripleId->getSubject());
        currentTriple->set_predicate(tripleId->getPredicate());
        currentTriple->set_object(tripleId->getObject());
        eraseDeletedVersions(triple_versions->get_versions(), currentTriple, first_version);
        return true;
    }

    // If we only have a snapshot, don't query the unavailable patch tree.
    if (patchTree == NULL) {
        return false;
    }

    // When we get here, no snapshot elements are left, so emit additions here
    if (addition_it == NULL) {
        addition_it = patchTree->addition_iterator(triple_pattern);
    }

    PatchTreeAdditionValue value;
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


size_t TripleVersionsIterator::get_count() {
    size_t count = 0;
    TripleVersions tv;
    while (next(&tv)) count++;
    return count;
}


TripleVersionsIterator* TripleVersionsIterator::offset(int offset) {
    TripleVersions tv;
    while(offset-- > 0 && next(&tv));
    return this;
}


TripleVersionsIteratorCombined::TripleVersionsIteratorCombined() : index(0) {}

void TripleVersionsIteratorCombined::append_iterator(TripleVersionsIterator *iterator, DictionaryManager *dict) {
    TripleVersions t;
    std::vector<TripleVersionsString> tmp_triples;
    while (iterator->next(&t)) {
        TemporaryTriple ts(t.get_triple()->get_subject(*dict), t.get_triple()->get_predicate(*dict),
                           t.get_triple()->get_object(*dict));
        tmp_triples.emplace_back(ts, *t.get_versions());
    }
    delete iterator;

    std::sort(tmp_triples.begin(), tmp_triples.end());

    std::vector<TripleVersionsString> sorted_out;
    auto it1 = triples.begin();
    auto it2 = tmp_triples.begin();
    while (it1 != triples.end()) {
        if (it2 == tmp_triples.end()) {
            sorted_out.insert(sorted_out.end(), it1, triples.end());
            break;
        }
        if (*it2 < *it1) {
            sorted_out.push_back(*it2);
            ++it2;
        } else {
            if (*it1->get_triple() == *it2->get_triple()) {
                std::vector<int> tmp_vec(*(*it2).get_versions());
                std::vector<int> tmp_out;
                (*it2).get_versions()->clear();
                std::merge(tmp_vec.begin(), tmp_vec.end(), (*it1).get_versions()->begin(), (*it1).get_versions()->end(),
                           std::back_inserter(tmp_out));
//                std::sort(tmp_vec.begin(), tmp_vec.end());
                auto erase_it = std::unique(tmp_out.begin(), tmp_out.end());
                tmp_out.erase(erase_it, tmp_out.end());
                (*it2).get_versions()->insert((*it2).get_versions()->begin(), tmp_out.begin(), tmp_out.end());
                sorted_out.push_back(*it2);
                ++it1;
                ++it2;
            } else {
                sorted_out.push_back(*it1);
                ++it1;
            }
        }
    }
    sorted_out.insert(sorted_out.end(), it2, tmp_triples.end());

    triples.clear();
    triples.insert(triples.begin(), sorted_out.begin(), sorted_out.end());
}

bool TripleVersionsIteratorCombined::next(TripleVersionsString *triple_versions) {
    if (index == triples.size())
        return false;
    TemporaryTriple *currentTriple = triples[index].get_triple();
    triple_versions->get_triple()->set_subject(currentTriple->get_subject());
    triple_versions->get_triple()->set_predicate(currentTriple->get_predicate());
    triple_versions->get_triple()->set_object(currentTriple->get_object());
    triple_versions->get_versions()->clear();
    triple_versions->get_versions()->insert(triple_versions->get_versions()->begin(),
                                            triples[index].get_versions()->begin(),
                                            triples[index].get_versions()->end());
    index++;
    return true;
}

size_t TripleVersionsIteratorCombined::get_count() {
    return triples.size();
}

TripleVersionsIteratorCombined *TripleVersionsIteratorCombined::offset(int offset) {
    index += offset;
    return this;
}


TripleVersionsString::TripleVersionsString() = default;

TripleVersionsString::TripleVersionsString(TemporaryTriple triple, std::vector<int> versions) : triple(
        std::move(triple)), versions(std::move(versions)) {}

TemporaryTriple *TripleVersionsString::get_triple() {
    return &triple;
}

std::vector<int> *TripleVersionsString::get_versions() {
    return &versions;
}

bool TripleVersionsString::operator<(const TripleVersionsString &other) const {
    return (triple < other.triple);
}
