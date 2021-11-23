#include "triple_delta_iterator.h"

TripleDelta::TripleDelta() : triple(new Triple()), addition(true), dict(nullptr) {}

TripleDelta::TripleDelta(Triple* triple, bool addition) : triple(triple), addition(addition), dict(nullptr) {}

Triple* TripleDelta::get_triple() {
    return triple;
}

bool TripleDelta::is_addition() {
    return addition;
}

TripleDelta::~TripleDelta() {
    delete triple;
}

void TripleDelta::set_addition(bool addition) {
    this->addition = addition;
}

std::shared_ptr<DictionaryManager> TripleDelta::get_dictionary() {
    return dict;
}

void TripleDelta::set_dictionary(std::shared_ptr<DictionaryManager> dictionary) {
    dict = dictionary;
}


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
    value = new PatchTreeValueBase<DV>();
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
    // and it was present in the snapshot, that it will be skipped.
    while ((valid = this->it->next(triple->get_triple(), this->value))
           && (!(addition = value->is_addition(it->get_patch_id_filter(), true)) && !value->exists_in_snapshot())) {}
    if (valid) {
        triple->set_addition(addition);
    }
    triple->set_dictionary(dict);
    return valid;
}

template <class DV>
FowardDiffPatchTripleDeltaIterator<DV>::FowardDiffPatchTripleDeltaIterator(std::shared_ptr<PatchTree> patchTree, const Triple &triple_pattern, int patch_id_start, int patch_id_end, std::shared_ptr<DictionaryManager> dict)
        : ForwardPatchTripleDeltaIterator<DV>(patchTree, triple_pattern, patch_id_end, dict), patch_id_start(patch_id_start), patch_id_end(patch_id_end) {
    this->it->set_filter_local_changes(false);
}

template <class DV>
bool FowardDiffPatchTripleDeltaIterator<DV>::next(TripleDelta *triple) {
    bool valid;
    while ((valid = this->it->next(triple->get_triple(), this->value))
                    && this->value->is_delta_type_equal(patch_id_start, patch_id_end)) {}
    if (valid) {
        triple->set_addition(this->value->is_addition(patch_id_end, true));
    }
    triple->set_dictionary(this->dict);
    return valid;
}


template class ForwardPatchTripleDeltaIterator<PatchTreeDeletionValue>;
template class ForwardPatchTripleDeltaIterator<PatchTreeDeletionValueReduced>;
template class FowardDiffPatchTripleDeltaIterator<PatchTreeDeletionValue>;
template class FowardDiffPatchTripleDeltaIterator<PatchTreeDeletionValueReduced>;


SnapshotDiffIterator::SnapshotDiffIterator(const StringTriple &triple_pattern, SnapshotManager *manager, int snapshot_1,
                                           int snapshot_2): t1(nullptr), t2(nullptr) {
    std::shared_ptr<HDT> snap_1 = manager->get_snapshot(snapshot_1);
    dict1 = manager->get_dictionary_manager(snapshot_1);
    std::shared_ptr<HDT> snap_2 = manager->get_snapshot(snapshot_2);
    dict2 = manager->get_dictionary_manager(snapshot_2);
    if (snap_1 && snap_2) {
        TripleString tp(triple_pattern.get_subject(), triple_pattern.get_predicate(), triple_pattern.get_object());
        snapshot_it_1 = snap_1->search(tp);
        snapshot_it_2 = snap_2->search(tp);
        if (snapshot_it_1->hasNext()) t1 = snapshot_it_1->next();
        if (snapshot_it_2->hasNext()) t2 = snapshot_it_2->next();
    }
}

int SnapshotDiffIterator::compare_ts(TripleString *ts1, TripleString *ts2) {
    int s_comp = ts1->getSubject().compare(ts2->getSubject());
    if (s_comp == 0) {
        int p_comp = ts1->getPredicate().compare(ts2->getPredicate());
        if (p_comp == 0) {
            return ts1->getObject().compare(ts2->getObject());
        }
        return p_comp;
    }
    return s_comp;
}

bool SnapshotDiffIterator::next(TripleDelta *triple) {
    auto emit_triple = [=](bool is_addition) {
        Triple* tmp_t;
        if (!is_addition) {
            tmp_t = new Triple(t1->getSubject(), t1->getPredicate(), t1->getObject(), dict1);
            triple->set_dictionary(dict1);
        } else {
            tmp_t = new Triple(t2->getSubject(), t2->getPredicate(), t2->getObject(), dict2);
            triple->set_dictionary(dict2);
        }
        triple->get_triple()->set_subject(tmp_t->get_subject());
        triple->get_triple()->set_predicate(tmp_t->get_predicate());
        triple->get_triple()->set_object(tmp_t->get_object());
        triple->set_addition(is_addition);
    };

    if (t1 && t2) {
        int comp = compare_ts(t1, t2);
        if (comp == 0) {
            t1 = snapshot_it_1->hasNext() ? snapshot_it_1->next() : nullptr;
            t2 = snapshot_it_2->hasNext() ? snapshot_it_2->next() : nullptr;
            return next(triple);
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


MergeDiffIterator::MergeDiffIterator(TripleDeltaIterator *iterator_1, TripleDeltaIterator *iterator_2) : it1(iterator_1),
        it2(iterator_2), triple1(new TripleDelta), triple2(new TripleDelta) {
    status1 = it1->next(triple1);
    status2 = it2->next(triple2);
}

//int compare_triple_delta(TripleDelta *td1, TripleDelta *td2) {
//    std::shared_ptr<DictionaryManager> dict1 = td1->get_dictionary() ? td1->get_dictionary() : td2->get_dictionary();
//    std::shared_ptr<DictionaryManager> dict2 = td2->get_dictionary() ? td2->get_dictionary() : td1->get_dictionary();
//    int s_comp = td1->get_triple()->get_subject(*dict1).compare(td2->get_triple()->get_subject(*dict2));
//    if (s_comp == 0) {
//        int p_comp = td1->get_triple()->get_predicate(*dict1).compare(td2->get_triple()->get_predicate(*dict2));
//        if (p_comp == 0) {
//            return td1->get_triple()->get_object(*dict1).compare(td2->get_triple()->get_object(*dict2));
//        }
//        return p_comp;
//    }
//    return s_comp;
//}

int compare_triple_delta(TripleDelta *td1, TripleDelta *td2) {
    size_t max_id = (size_t) -1;
    size_t mask = 1ULL << 63;
    bool is_same_dict = td1->get_dictionary() == td2->get_dictionary();
    std::shared_ptr<DictionaryManager> dict1 = td1->get_dictionary() ? td1->get_dictionary() : td2->get_dictionary();
    std::shared_ptr<DictionaryManager> dict2 = td2->get_dictionary() ? td2->get_dictionary() : td1->get_dictionary();
    int s_comp;
    if (is_same_dict && !(td1->get_triple()->get_subject() & mask) && !(td2->get_triple()->get_subject() & mask)) {
        s_comp = dict1->compareComponent(td1->get_triple()->get_subject(), td2->get_triple()->get_subject(), SUBJECT);
    } else {
        s_comp = td1->get_triple()->get_subject(*dict1).compare(td2->get_triple()->get_subject(*dict2));
    }
    if (s_comp == 0) {
        int p_comp;
        if (is_same_dict && !(td1->get_triple()->get_predicate() & mask) && !(td2->get_triple()->get_predicate() & mask)) {
            p_comp = dict1->compareComponent(td1->get_triple()->get_predicate(), td2->get_triple()->get_predicate(), PREDICATE);
        } else {
            p_comp = td1->get_triple()->get_predicate(*dict1).compare(td2->get_triple()->get_predicate(*dict2));
        }
        if (p_comp == 0) {
            int o_comp;
            if (is_same_dict && !(td1->get_triple()->get_object() & mask) && !(td2->get_triple()->get_object() & mask)) {
                o_comp = dict1->compareComponent(td1->get_triple()->get_object(), td2->get_triple()->get_object(), OBJECT);
            } else {
                o_comp = td1->get_triple()->get_object(*dict1).compare(td2->get_triple()->get_object(*dict2));
            }
            return o_comp;
        }
        return p_comp;
    }
    return s_comp;
}

bool MergeDiffIterator::next(TripleDelta *triple) {
    auto emit_triple = [](TripleDelta* source, TripleDelta* target, bool is_addition) {
        target->get_triple()->set_subject(source->get_triple()->get_subject());
        target->get_triple()->set_predicate(source->get_triple()->get_predicate());
        target->get_triple()->set_object(source->get_triple()->get_object());
        target->set_addition(is_addition);
        target->set_dictionary(source->get_dictionary());
    };

    if (status1 && status2) {
        int comp = compare_triple_delta(triple1, triple2);
        if (comp == 0) {  // It's the same triple (SPO)
            if (triple2->is_addition()) {
                emit_triple(triple2, triple, true);
            } else {
                emit_triple(triple2, triple, false);
            }
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
}


SortedTripleDeltaIterator::SortedTripleDeltaIterator(TripleDeltaIterator *iterator): index(0) {
    auto comp = [](TripleDelta* td1, TripleDelta* td2) {
        int comp_res = compare_triple_delta(td1, td2);
        return comp_res < 0;
    };

    auto td = new TripleDelta;
    while(iterator->next(td)) {
        triples.emplace_back(td);
        td = new TripleDelta;
    }
    if (!std::is_sorted(triples.begin(), triples.end(), comp))
        std::sort(triples.begin(), triples.end(), comp);
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


MergeDiffIteratorCase2::MergeDiffIteratorCase2(TripleDeltaIterator *iterator_1, TripleDeltaIterator *iterator_2):
    it1(iterator_1), it2(iterator_2), triple1(new TripleDelta), triple2(new TripleDelta)
{
    status1 = it1->next(triple1);
    status2 = it2->next(triple2);
}

MergeDiffIteratorCase2::~MergeDiffIteratorCase2() {
    delete it1;
    delete it2;
    delete triple1;
    delete triple2;
}

bool MergeDiffIteratorCase2::next(TripleDelta *triple) {
    auto emit_triple = [](TripleDelta* source, TripleDelta* target, bool is_addition) {
        target->get_triple()->set_subject(source->get_triple()->get_subject());
        target->get_triple()->set_predicate(source->get_triple()->get_predicate());
        target->get_triple()->set_object(source->get_triple()->get_object());
        target->set_addition(is_addition);
        target->set_dictionary(source->get_dictionary());
    };

    if (status1 && status2) {
        int comp = compare_triple_delta(triple1, triple2);
        if (comp == 0) {  // It's the same triple (SPO)
            if (triple1->is_addition() != triple2->is_addition()) {
                emit_triple(triple2, triple, triple2->is_addition());
                status1 = it1->next(triple1);
                status2 = it2->next(triple2);
                return true;
            } else {
                status1 = it1->next(triple1);
                status2 = it2->next(triple2);
                return next(triple);
            }
        } else if (comp < 0) {
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
    std::map<int, std::shared_ptr<HDT>> snapshots = snapshot_manager->get_snapshots();
    std::map<int, std::shared_ptr<PatchTree>> patch_trees = patch_tree_manager->get_patch_trees();
    int min_id = std::min(snapshot_id_1, snapshot_id_2);
    int max_id = std::max(snapshot_id_1, snapshot_id_2);
    auto it1 = snapshots.find(min_id);
    auto it2 = snapshots.find(max_id);
    if (it1 == snapshots.end() || it2 == snapshots.end()) {
        throw std::runtime_error("could not find the snapshots to compute diff");
    }
    std::vector<int> snapshots_ids;
    while (it1 != it2) {
        snapshots_ids.push_back(it1->first);
        it1++;
    }
    snapshots_ids.push_back(it1->first);
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
            tmp = new SortedTripleDeltaIterator(unsorted);
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
    std::map<int, std::shared_ptr<HDT>> snapshots = snapshot_manager->get_snapshots();
    std::map<int, std::shared_ptr<PatchTree>> patch_trees = patch_tree_manager->get_patch_trees();
    int min_id = std::min(snapshot_id_1, snapshot_id_2);
    int max_id = std::max(snapshot_id_1, snapshot_id_2);
    auto it1 = snapshots.find(min_id);
    auto it2 = snapshots.find(max_id);
    if (it1 == snapshots.end() || it2 == snapshots.end()) {
        throw std::runtime_error("could not find the snapshots to compute diff");
    }

    // Heuristic
    size_t distance = std::distance(it1, it2);
    TripleString tp(triple_pattern.get_subject(), triple_pattern.get_predicate(), triple_pattern.get_object());
    auto hdt_it = it2->second->search(tp);
    size_t est = hdt_it->estimatedNumResults();
    // TODO: refine the heuristic
    bool use_iterative = distance > 8 || est < 1000;
    if (use_iterative) {
        internal_it = new SnapshotDiffIterator(triple_pattern, snapshot_manager, snapshot_id_1, snapshot_id_2);
    } else {
        internal_it = new IterativeSnapshotDiffIterator(triple_pattern, snapshot_manager, patch_tree_manager, snapshot_id_1, snapshot_id_2);
    }
}

AutoSnapshotDiffIterator::~AutoSnapshotDiffIterator() {
    delete internal_it;
}

bool AutoSnapshotDiffIterator::next(TripleDelta *triple) {
    return internal_it->next(triple);
}
