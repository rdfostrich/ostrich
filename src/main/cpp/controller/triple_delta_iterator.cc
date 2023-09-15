#include "triple_delta_iterator.h"



TripleDeltaIterator::~TripleDeltaIterator() {}

TripleDeltaIterator* TripleDeltaIterator::offset(int offset) {
    TripleDelta td;
    while(offset-- > 0 && next(&td));
    return this;
}

size_t TripleDeltaIterator::get_count() {
    size_t count = 0;
    TripleDelta td;
    while (next(&td)) count++;
    return count;
}


bool EmptyTripleDeltaIterator::next(TripleDelta *triple) {
    return false;
}

template <class DV>
ForwardPatchTripleDeltaIterator<DV>::ForwardPatchTripleDeltaIterator(std::shared_ptr<PatchTree> patchTree, const Triple &triple_pattern, int patch_id_end, std::shared_ptr<DictionaryManager> dict) : it(patchTree->iterator<DV>(&triple_pattern)), dict(dict) {
    it->set_patch_filter(patch_id_end, false);
    it->set_filter_local_changes(true);
    it->set_early_break(true);
    it->set_squash_equal_addition_deletion(true);
#if defined(COMPRESSED_ADD_VALUES) || defined(COMPRESSED_DEL_VALUES)
    value = new PatchTreeValueBase<DV>(patchTree->get_max_patch_id());
#else
    value = new PatchTreeValueBase<DV>();
#endif
}

template <class DV>
ForwardPatchTripleDeltaIterator<DV>::~ForwardPatchTripleDeltaIterator() {
    delete it;
    delete value;
}

template <class DV>
bool ForwardPatchTripleDeltaIterator<DV>::next(TripleDelta* triple) {
    bool valid, addition;
    // This loop makes sure that if the triple is a deletion,
    // and it was not present in the snapshot, that it will be skipped.
    while ((valid = this->it->next(triple->get_triple(), this->value)) // we have a triple (it valid)
           && (!(addition = value->is_addition(it->get_patch_id_filter(), true)) // triple is a deletion
           && !value->exists_in_snapshot())) {} // the triple does not exist in the snapshot
    if (valid) {
        triple->set_addition(addition);
    }
    triple->set_dictionary(dict);
    return valid;
}

template <class DV>
ForwardDiffPatchTripleDeltaIterator<DV>::ForwardDiffPatchTripleDeltaIterator(std::shared_ptr<PatchTree> patchTree, const Triple &triple_pattern, int patch_id_start, int patch_id_end, std::shared_ptr<DictionaryManager> dict)
        : ForwardPatchTripleDeltaIterator<DV>(patchTree, triple_pattern, patch_id_end, dict), patch_id_start(patch_id_start), patch_id_end(patch_id_end) {
    this->it->set_filter_local_changes(false);
}

template <class DV>
bool ForwardDiffPatchTripleDeltaIterator<DV>::next(TripleDelta *triple) {
    bool valid;
    while ((valid = this->it->next(triple->get_triple(), this->value))  // we have a triple (it valid)
                    && this->value->is_delta_type_equal(patch_id_start, patch_id_end)) {}  // the triple exist in both version (so not a delta)
    if (valid) {
        triple->set_addition(this->value->is_addition(patch_id_end, true));
    }
    triple->set_dictionary(this->dict);
    return valid;
}


template class ForwardPatchTripleDeltaIterator<PatchTreeDeletionValue>;
template class ForwardPatchTripleDeltaIterator<PatchTreeDeletionValueReduced>;
template class ForwardDiffPatchTripleDeltaIterator<PatchTreeDeletionValue>;
template class ForwardDiffPatchTripleDeltaIterator<PatchTreeDeletionValueReduced>;


SnapshotDiffIterator::SnapshotDiffIterator(const StringTriple &triple_pattern, SnapshotManager *manager, int snapshot_1,
                                           int snapshot_2, hdt::TripleComponentOrder qr_order): t1(nullptr), t2(nullptr), comparator(TripleComparator::get_triple_comparator(qr_order)) {
    std::shared_ptr<hdt::HDT> snap_1 = manager->get_snapshot(snapshot_1);
    dict1 = manager->get_dictionary_manager(snapshot_1);
    std::shared_ptr<hdt::HDT> snap_2 = manager->get_snapshot(snapshot_2);
    dict2 = manager->get_dictionary_manager(snapshot_2);
    if (snap_1 && snap_2) {
        Triple tp1 = triple_pattern.get_as_triple(dict1);
        Triple tp2 = triple_pattern.get_as_triple(dict2);
        snapshot_it_1 = SnapshotManager::search_with_offset(snap_1, tp1, 0, dict1, true);
        snapshot_it_2 = SnapshotManager::search_with_offset(snap_2, tp2, 0, dict2, true);
        if (snapshot_it_1->hasNext()) t1 = snapshot_it_1->next();
        if (snapshot_it_2->hasNext()) t2 = snapshot_it_2->next();
    }
}

bool SnapshotDiffIterator::next(TripleDelta *triple) {
    auto emit_triple = [=](bool is_addition) {
        Triple* tmp_t;
        if (!is_addition) {
            tmp_t = new Triple(t1->getSubject(), t1->getPredicate(), t1->getObject());
            triple->set_dictionary(dict1);
        } else {
            tmp_t = new Triple(t2->getSubject(), t2->getPredicate(), t2->getObject());
            triple->set_dictionary(dict2);
        }
        triple->get_triple()->set_subject(tmp_t->get_subject());
        triple->get_triple()->set_predicate(tmp_t->get_predicate());
        triple->get_triple()->set_object(tmp_t->get_object());
        triple->set_addition(is_addition);
        delete tmp_t;
    };

    if (t1 && t2) {
        int comp = comparator->compare(*t1, *t2, dict1, dict2);
        if (comp == 0) {
            while (comp == 0 && t1 && t2) {
                t1 = snapshot_it_1->hasNext() ? snapshot_it_1->next() : nullptr;
                t2 = snapshot_it_2->hasNext() ? snapshot_it_2->next() : nullptr;
                if (t1 && t2) {
                    comp = comparator->compare(*t1, *t2, dict1, dict2);
                }
            }
            if (comp < 0 || (t1 && !t2)) {
                emit_triple(false);
                t1 = snapshot_it_1->hasNext() ? snapshot_it_1->next() : nullptr;
            } else if (comp > 0 || (!t1 && t2)) {
                emit_triple(true);
                t2 = snapshot_it_2->hasNext() ? snapshot_it_2->next() : nullptr;
            } else {
                return false;
            }
            return true;
        } else if (comp < 0) {
            // t1 is a deletion
            emit_triple(false);
            t1 = snapshot_it_1->hasNext() ? snapshot_it_1->next() : nullptr;
            return true;
        } else {
            // t2 is an addition
            emit_triple(true);
            t2 = snapshot_it_2->hasNext() ? snapshot_it_2->next() : nullptr;
            return true;
        }
    } else if (t1) {
        emit_triple(false);
        t1 = snapshot_it_1->hasNext() ? snapshot_it_1->next() : nullptr;
        return true;
    } else if (t2) {
        emit_triple(true);
        t2 = snapshot_it_2->hasNext() ? snapshot_it_2->next() : nullptr;
        return true;
    } else {
        return false;
    }
}

SnapshotDiffIterator::~SnapshotDiffIterator() {
    delete snapshot_it_1;
    delete snapshot_it_2;
    delete t1;
    delete t2;
    delete comparator;
}


MergeDiffIterator::MergeDiffIterator(TripleDeltaIterator *iterator_1, TripleDeltaIterator *iterator_2, hdt::TripleComponentOrder qr_order) : it1(iterator_1),
        it2(iterator_2), triple1(new TripleDelta), triple2(new TripleDelta), comparator(TripleComparator::get_triple_comparator(qr_order)) {
    status1 = it1->next(triple1);
    status2 = it2->next(triple2);
}

bool MergeDiffIterator::next(TripleDelta *triple) {
    auto emit_triple = [](TripleDelta* source, TripleDelta* target, bool is_addition) {
        target->get_triple()->set_subject(source->get_triple()->get_subject());
        target->get_triple()->set_predicate(source->get_triple()->get_predicate());
        target->get_triple()->set_object(source->get_triple()->get_object());
        target->set_addition(is_addition);
        target->set_dictionary(source->get_dictionary());
    };

    // If both iterators are valid
    if (status1 && status2) {
        int comp = comparator->compare(triple1, triple2);
        // we need to skip identical triples if they have different addition flags
        // before handing out to the rest of the function
        while (comp == 0 && status1 && status2 && (triple1->is_addition() != triple2->is_addition())) {
            status1 = it1->next(triple1);
            status2 = it2->next(triple2);
            if (status1 && status2) {
                comp = comparator->compare(triple1, triple2);
            }
        }
    }

    if (status1 && status2) {
        int comp = comparator->compare(triple1, triple2);
        if (comp == 0) {
            emit_triple(triple2, triple, triple2->is_addition());
            status1 = it1->next(triple1);
            status2 = it2->next(triple2);
            return true;
        } else if (comp < 0) {
            emit_triple(triple1, triple, triple1->is_addition());
            status1 = it1->next(triple1);
            return true;
        } else {
            emit_triple(triple2, triple, triple2->is_addition());
            status2 = it2->next(triple2);
            return true;
        }
    } else if (status1) {
        emit_triple(triple1, triple, triple1->is_addition());
        status1 = it1->next(triple1);
        return true;
    } else if (status2) {
        emit_triple(triple2, triple, triple2->is_addition());
        status2 = it2->next(triple2);
        return true;
    }
    return false;
}

MergeDiffIterator::~MergeDiffIterator() {
    delete it1;
    delete it2;
    delete triple1;
    delete triple2;
    delete comparator;
}


SortedTripleDeltaIterator::SortedTripleDeltaIterator(TripleDeltaIterator *iterator, hdt::TripleComponentOrder order): index(0) {
    auto td = new TripleDelta;
    while(iterator->next(td)) {
        triples.emplace_back(td);
        td = new TripleDelta;
    }
    delete td;
    delete iterator;
    TripleComparator* comparator = TripleComparator::get_triple_comparator(order);
    if (!std::is_sorted(triples.begin(), triples.end(), *comparator))
        std::sort(triples.begin(), triples.end(), *comparator);
    delete comparator;
}

bool SortedTripleDeltaIterator::next(TripleDelta *triple) {
    if (index < triples.size()) {
        triple->get_triple()->set_subject(triples[index]->get_triple()->get_subject());
        triple->get_triple()->set_predicate(triples[index]->get_triple()->get_predicate());
        triple->get_triple()->set_object(triples[index]->get_triple()->get_object());
        triple->set_addition(triples[index]->is_addition());
        triple->set_dictionary(triples[index]->get_dictionary());
        index++;
        return true;
    }
    return false;
}

SortedTripleDeltaIterator::~SortedTripleDeltaIterator() {
    for (auto triple : triples) {
        delete triple;
    }
}


MergeDiffIteratorCase2::MergeDiffIteratorCase2(TripleDeltaIterator *iterator_1, TripleDeltaIterator *iterator_2, hdt::TripleComponentOrder qr_order):
    it1(iterator_1), it2(iterator_2), triple1(new TripleDelta), triple2(new TripleDelta), comparator(TripleComparator::get_triple_comparator(qr_order))
{
    status1 = it1->next(triple1);
    status2 = it2->next(triple2);
}

MergeDiffIteratorCase2::~MergeDiffIteratorCase2() {
    delete it1;
    delete it2;
    delete triple1;
    delete triple2;
    delete comparator;
}

bool MergeDiffIteratorCase2::next(TripleDelta *triple) {
    auto emit_triple = [](TripleDelta* source, TripleDelta* target, bool is_addition) {
        target->get_triple()->set_subject(source->get_triple()->get_subject());
        target->get_triple()->set_predicate(source->get_triple()->get_predicate());
        target->get_triple()->set_object(source->get_triple()->get_object());
        target->set_addition(is_addition);
        target->set_dictionary(source->get_dictionary());
    };

    // If both iterators are valid
    if (status1 && status2) {
        int comp = comparator->compare(triple1, triple2);
        // we need to skip identical triples if they have identical addition flags
        // before handing out to the rest of the function
        // in practice we skip all identical triples regardless of flag since different flags should never happen
        while (comp == 0 && status1 && status2) { //&& (triple1->is_addition() == triple2->is_addition())) {
            status1 = it1->next(triple1);
            status2 = it2->next(triple2);
            if (status1 && status2) {
                comp = comparator->compare(triple1, triple2);
            }
        }
    }

    if (status1 && status2) {
        int comp = comparator->compare(triple1, triple2);
        if (comp < 0) {
            // The triple from set 1 does not exist in set 2
            // It means that it must have been reverted somewhere in between
            // So if triple1 is addition, then it's a deletion
            // and if triple1 is deletion, then it's an addition
            emit_triple(triple1, triple, !triple1->is_addition());
            status1 = it1->next(triple1);
            return true;
        } else {
            emit_triple(triple2, triple, triple2->is_addition());
            status2 = it2->next(triple2);
            return true;
        }
    } else if (status1) {
        emit_triple(triple1, triple, !triple1->is_addition());
        status1 = it1->next(triple1);
        return true;
    } else if (status2) {
        emit_triple(triple2, triple, triple2->is_addition());
        status2 = it2->next(triple2);
        return true;
    }
    return false;
}


IterativeSnapshotDiffIterator::IterativeSnapshotDiffIterator(const StringTriple& triple_pattern, SnapshotManager *snapshot_manager,
                                                             PatchTreeManager *patch_tree_manager, int snapshot_id_1,
                                                             int snapshot_id_2): internal_it(nullptr) {

    int min_id = std::min(snapshot_id_1, snapshot_id_2);
    int max_id = std::max(snapshot_id_1, snapshot_id_2);
    std::vector<int> snapshots_ids;
    std::vector<int> snapshots = snapshot_manager->get_snapshots_ids();
    auto it1 = std::find(snapshots.begin(), snapshots.end(), min_id);
    auto it2 = std::find(snapshots.begin(), snapshots.end(), max_id);
    if (it1 == snapshots.end() || it2 == snapshots.end()) {
        throw std::runtime_error("could not find the snapshots to compute diff");
    }
    while (it1 != it2) {
        snapshots_ids.push_back(*it1);
        it1++;
    }
    snapshots_ids.push_back(*it1);

    TripleDeltaIterator* start_it = nullptr;
    MergeDiffIterator* it = nullptr;
    for (int i = 1; i < snapshots_ids.size(); i++) {
        int id = patch_tree_manager->get_patch_tree_id(snapshots_ids[i]);
        std::shared_ptr<DictionaryManager> dict = snapshot_manager->get_dictionary_manager(snapshots_ids[i-1]);
        std::shared_ptr<PatchTree> pt = patch_tree_manager->get_patch_tree(id, dict);
        TripleDeltaIterator* tmp;
        Triple tp = triple_pattern.get_as_triple(dict);
        if (TripleStore::is_default_tree(tp)) {
            tmp = new ForwardPatchTripleDeltaIterator<PatchTreeDeletionValue>(pt, tp, snapshots_ids[i], dict);
        } else {
            TripleDeltaIterator* unsorted = new ForwardPatchTripleDeltaIterator<PatchTreeDeletionValueReduced>(pt, tp, snapshots_ids[i], dict);
            tmp = new SortedTripleDeltaIterator(unsorted, hdt::SPO);
            delete unsorted;
        }
        if (start_it == nullptr) {
            start_it = tmp;
        } else if (it == nullptr) {
            it = new MergeDiffIterator(start_it, tmp);
        } else {
            it = new MergeDiffIterator(it, tmp);
        }
    }
    if (it) {
        internal_it = it;
    } else if(start_it) {
        internal_it = start_it;
    } else {
        throw std::runtime_error("cannot create iterator for snapshots " + std::to_string(snapshot_id_1) + " and " + std::to_string(snapshot_id_2));
    }
}

bool IterativeSnapshotDiffIterator::next(TripleDelta *triple) {
    return internal_it->next(triple);
}

IterativeSnapshotDiffIterator::~IterativeSnapshotDiffIterator() {
    delete internal_it;
}

AutoSnapshotDiffIterator::AutoSnapshotDiffIterator(const StringTriple &triple_pattern,
                                                   SnapshotManager *snapshot_manager,
                                                   PatchTreeManager *patch_tree_manager, int snapshot_id_1,
                                                   int snapshot_id_2) {
    int min_id = std::min(snapshot_id_1, snapshot_id_2);
    int max_id = std::max(snapshot_id_1, snapshot_id_2);
    std::vector<int> snapshots = snapshot_manager->get_snapshots_ids();
    auto it1 = std::find(snapshots.begin(), snapshots.end(), min_id);
    auto it2 = std::find(snapshots.begin(), snapshots.end(), max_id);
    if (it1 == snapshots.end() || it2 == snapshots.end()) {
        throw std::runtime_error("could not find the snapshots to compute diff");
    }
    size_t distance = std::distance(it1, it2);
    hdt::TripleString tp(triple_pattern.get_subject(), triple_pattern.get_predicate(), triple_pattern.get_object());
    if (distance <= 1) {
        std::shared_ptr<DictionaryManager> dict = snapshot_manager->get_dictionary_manager(min_id);
        Triple ttp = triple_pattern.get_as_triple(dict);
        std::shared_ptr<PatchTree> patch_tree = patch_tree_manager->get_patch_tree(patch_tree_manager->get_patch_tree_id(max_id), dict);
        if (TripleStore::is_default_tree(ttp)) {
            internal_it = new ForwardPatchTripleDeltaIterator<PatchTreeDeletionValue>(patch_tree, ttp, max_id, dict);
        } else {
            internal_it = new ForwardPatchTripleDeltaIterator<PatchTreeDeletionValueReduced>(patch_tree, ttp, max_id, dict);
        }
    } else {
        internal_it = new SnapshotDiffIterator(triple_pattern, snapshot_manager, snapshot_id_1, snapshot_id_2, TripleStore::get_query_order(triple_pattern));
    }
}

AutoSnapshotDiffIterator::~AutoSnapshotDiffIterator() {
    delete internal_it;
}

bool AutoSnapshotDiffIterator::next(TripleDelta *triple) {
    return internal_it->next(triple);
}


PlainDiffDeltaIterator::PlainDiffDeltaIterator(TripleIterator *it1, TripleIterator *it2,
                                               std::shared_ptr<DictionaryManager> dict1,
                                               std::shared_ptr<DictionaryManager> dict2) : it_v1(it1), it_v2(it2),
                                                                                           dict1(dict1), dict2(dict2),
                                                                                           triple1(new Triple),
                                                                                           triple2(new Triple),
                                                                                           comparator(TripleComparator::get_triple_comparator(hdt::SPO, dict1, dict2)) {
    status1 = it_v1->next(triple1);
    status2 = it_v2->next(triple2);
}

PlainDiffDeltaIterator::~PlainDiffDeltaIterator() {
    delete it_v1;
    delete it_v2;
    delete triple1;
    delete triple2;
}

bool PlainDiffDeltaIterator::next(TripleDelta *triple) {
    auto emit_triple = [=](Triple* source, bool is_addition, std::shared_ptr<DictionaryManager> dict) {
        triple->get_triple()->set_subject(source->get_subject());
        triple->get_triple()->set_predicate(source->get_predicate());
        triple->get_triple()->set_object(source->get_object());
        triple->set_addition(is_addition);
        triple->set_dictionary(dict);
    };

    if (!status1 && !status2) {
        return false;
    }
    if (!status1 && status2) {
        emit_triple(triple2, true, dict2);
        status2 = it_v2->next(triple2);
        return true;
    }
    if (status1 && !status2) {
        emit_triple(triple1, false, dict1);
        status1 = it_v1->next(triple1);
        return true;
    }
    int comp = comparator->compare(*triple1, *triple2);
    if (comp == 0) {  // Triple exist in both version, we skip
        status1 = it_v1->next(triple1);
        status2 = it_v2->next(triple2);
        return next(triple);
    }
    if (comp < 0) { // Triple from v1 don't exist in v2, it's a deletion
        emit_triple(triple1, false, dict1);
        status1 = it_v1->next(triple1);
        return true;
    }
    if (comp > 0) { // Triple from v2 don't exist in v1, it's an addition
        emit_triple(triple2, true, dict2);
        status2 = it_v2->next(triple2);
        return true;
    }
    return false;
}
