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

DictionaryManager* TripleDelta::get_dictionary() {
    return dict;
}

void TripleDelta::set_dictionary(DictionaryManager *dictionary) {
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
ForwardPatchTripleDeltaIterator<DV>::ForwardPatchTripleDeltaIterator(PatchTree* patchTree, const Triple &triple_pattern, int patch_id_end) : it(patchTree->iterator<DV>(&triple_pattern)) {
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
    return valid;
}

template <class DV>
FowardDiffPatchTripleDeltaIterator<DV>::FowardDiffPatchTripleDeltaIterator(PatchTree* patchTree, const Triple &triple_pattern, int patch_id_start, int patch_id_end)
        : ForwardPatchTripleDeltaIterator<DV>(patchTree, triple_pattern, patch_id_end), patch_id_start(patch_id_start), patch_id_end(patch_id_end) {
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
    return valid;
}


template class ForwardPatchTripleDeltaIterator<PatchTreeDeletionValue>;
template class ForwardPatchTripleDeltaIterator<PatchTreeDeletionValueReduced>;
template class FowardDiffPatchTripleDeltaIterator<PatchTreeDeletionValue>;
template class FowardDiffPatchTripleDeltaIterator<PatchTreeDeletionValueReduced>;


SnapshotDiffIterator::SnapshotDiffIterator(TripleString &triple_pattern, SnapshotManager *manager, int snapshot_1,
                                           int snapshot_2): t1(nullptr), t2(nullptr) {
    HDT* snap_1 = manager->get_snapshot(snapshot_1);
    dict1 = manager->get_dictionary_manager(snapshot_1);
    HDT* snap_2 = manager->get_snapshot(snapshot_2);
    dict2 = manager->get_dictionary_manager(snapshot_2);
    if (snap_1 && snap_2) {
        snapshot_it_1 = snap_1->search(triple_pattern);
        snapshot_it_2 = snap_2->search(triple_pattern);
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
        triple->set_addition(false);
    };

    if (t1 && t2) {
        int comp = compare_ts(t1, t2);
        if (comp == 0) {
            t1 = snapshot_it_1->next();
            t2 = snapshot_it_1->next();
            return next(triple);
        } else if (comp < 0) {
            // t1 is a deletion
            emit_triple(false);
            t1 = snapshot_it_1->next();
            return true;
        } else {
            // t2 is an addition
            emit_triple(true);
            t2 = snapshot_it_2->next();
            return true;
        }
    } else if (t1) {
        emit_triple(false);
        t1 = snapshot_it_1->next();
        return true;
    } else if (t2) {
        emit_triple(true);
        t2 = snapshot_it_2->next();
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

int compare_triple_delta(TripleDelta *td1, TripleDelta *td2) {
    if (td1->get_dictionary() == td2->get_dictionary()) {
        int s_comp = td1->get_triple()->get_subject() - td2->get_triple()->get_subject();
        if (s_comp == 0) {
            int p_comp = td1->get_triple()->get_predicate() - td2->get_triple()->get_predicate();
            if (p_comp == 0) {
                return td1->get_triple()->get_object() - td2->get_triple()->get_object();
            }
            return p_comp;
        }
        return s_comp;
    } else {
        // We need to compare the Triple as string because ids are not (necessarily) the same
        // The two triples have two different dict
        // One of them can be unspecified (null) so it will use the dict from the other triple
        // Alternatively we could use the dict from snapshot 0
        DictionaryManager* dict1 = td1->get_dictionary() ? td1->get_dictionary() : td2->get_dictionary();
        DictionaryManager* dict2 = td2->get_dictionary() ? td2->get_dictionary() : td1->get_dictionary();
        int s_comp = td1->get_triple()->get_subject(*dict1).compare(td2->get_triple()->get_subject(*dict2));
        if (s_comp == 0) {
            int p_comp = td1->get_triple()->get_predicate(*dict1).compare(td2->get_triple()->get_predicate(*dict2));
            if (p_comp == 0) {
                return td1->get_triple()->get_object(*dict1).compare(td2->get_triple()->get_object(*dict2));
            }
            return p_comp;
        }
        return s_comp;
    }
}

bool MergeDiffIterator::next(TripleDelta *triple) {
    auto emit_triple = [](TripleDelta* source, TripleDelta* target, bool is_addition) {
        target->get_triple()->set_subject(source->get_triple()->get_subject());
        target->get_triple()->set_predicate(source->get_triple()->get_predicate());
        target->get_triple()->set_object(source->get_triple()->get_object());
        target->set_addition(is_addition);
        target->set_dictionary(target->get_dictionary());
    };

    if (status1 && status2) {
        int comp = compare_triple_delta(triple1, triple2);
        if (comp == 0) {  // It's the same triple (SPO)
            if (triple2->is_addition()) {
                emit_triple(triple2, triple, true);
            } else {
                emit_triple(triple2, triple, false);
            }
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


SortedTripleDeltaIterator::SortedTripleDeltaIterator(TripleDeltaIterator *iterator): index(0) {
    auto comp = [](TripleDelta& td1, TripleDelta& td2) {
        int comp_res = compare_triple_delta(&td1, &td2);
        return comp_res < 0;
    };

    TripleDelta td;
    while(iterator->next(&td)) {
        triples.emplace_back(td);
    }
    if (!std::is_sorted(triples.begin(), triples.end(), comp))
        std::sort(triples.begin(), triples.end(), comp);
}

bool SortedTripleDeltaIterator::next(TripleDelta *triple) {
    if (index < triples.size()) {
        triple->get_triple()->set_subject(triples[index].get_triple()->get_subject());
        triple->get_triple()->set_predicate(triples[index].get_triple()->get_predicate());
        triple->get_triple()->set_object(triples[index].get_triple()->get_object());
        triple->set_addition(triples[index].is_addition());
        triple->set_dictionary(triples[index].get_dictionary());
        index++;
        return true;
    }
    return false;
}
