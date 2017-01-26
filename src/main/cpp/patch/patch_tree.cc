#include <iostream>
#include <kchashdb.h>
#include <fstream>

#include "patch_tree.h"
#include "../simpleprogresslistener.h"

using namespace std;
using namespace kyotocabinet;

PatchTree::PatchTree(string basePath, int min_patch_id, DictionaryManager* dict, int8_t kc_opts)
        : metadata_filename(basePath + METADATA_FILENAME_BASE(min_patch_id)), min_patch_id(min_patch_id), max_patch_id(min_patch_id) {
    tripleStore = new TripleStore(basePath + PATCHTREE_FILENAME_BASE(min_patch_id), dict, kc_opts);
    read_metadata();
};

PatchTree::~PatchTree() {
    write_metadata();
    delete tripleStore;
}

void PatchTree::init_temp_insertion_trees(HashDB &sp_, HashDB &s_o, HashDB &s__, HashDB &_po, HashDB &_p_, HashDB &__o) {
    std::remove(".additions.sp_.tmp");
    std::remove(".additions.s_o.tmp");
    std::remove(".additions.s__.tmp");
    std::remove(".additions._po.tmp");
    std::remove(".additions._p_.tmp");
    std::remove(".additions.__o.tmp");

    sp_.open(".additions.sp_.tmp", HashDB::OWRITER | HashDB::OCREATE);
    s_o.open(".additions.s_o.tmp", HashDB::OWRITER | HashDB::OCREATE);
    s__.open(".additions.s__.tmp", HashDB::OWRITER | HashDB::OCREATE);
    _po.open(".additions._po.tmp", HashDB::OWRITER | HashDB::OCREATE);
    _p_.open(".additions._p_.tmp", HashDB::OWRITER | HashDB::OCREATE);
    __o.open(".additions.__o.tmp", HashDB::OWRITER | HashDB::OCREATE);
}

void PatchTree::deinit_temp_insertion_trees(HashDB &sp_, HashDB &s_o, HashDB &s__, HashDB &_po, HashDB &_p_, HashDB &__o) {
    sp_.close();
    s_o.close();
    s__.close();
    _po.close();
    _p_.close();
    __o.close();

    std::remove(".additions.sp_.tmp");
    std::remove(".additions.s_o.tmp");
    std::remove(".additions.s__.tmp");
    std::remove(".additions._po.tmp");
    std::remove(".additions._p_.tmp");
    std::remove(".additions.__o.tmp");
}

void PatchTree::append_unsafe(PatchElementIterator *patch_it, int patch_id, ProgressListener *progressListener) {
    const char *kbp, *vbp;
    size_t ksp, vsp;
    PatchTreeDeletionValue deletion_value;
    PatchTreeAdditionValue addition_value;
    PatchTreeKey deletion_key, addition_key;
    PatchElement patch_element(Triple(0, 0, 0), true);

    // Loop over SPO deletion and addition trees
    // We do this together to be able to efficiently determine the local change flags
    NOTIFYMSG(progressListener, "Inserting into deletion and addition trees...\n");
    DB::Cursor* cursor_deletions = tripleStore->getDeletionsTree()->cursor();
    DB::Cursor* cursor_additions = tripleStore->getDefaultAdditionsTree()->cursor();

    cursor_deletions->jump();
    cursor_additions->jump();

    // Counters for all possible patch positions
    // We use KC hashmaps to store the potentially large amounts of triple patterns to avoid running out of memory.
    HashDB sp_;
    HashDB s_o;
    HashDB s__;
    HashDB _po;
    HashDB _p_;
    HashDB __o;
    PatchPosition ___ = 0;
    init_temp_insertion_trees(sp_, s_o, s__, _po, _p_, __o);

    bool should_step_patch = true;
    bool should_step_deletions = true;
    bool should_step_additions = true;
    bool has_patch_ended = false;
    bool have_deletions_ended = false;
    bool have_additions_ended = false;
    long i = 0;
    while (true) {
        if (i % 10000 == 0) {
            NOTIFYLVL(progressListener, "Triple insertion", i);
        }
        if (should_step_patch) {
            has_patch_ended = !patch_it->next(&patch_element);
            i++;
        }
        if (should_step_deletions) {
            kbp = cursor_deletions->get(&ksp, &vbp, &vsp, false);
            have_deletions_ended = kbp == NULL;
            if (!have_deletions_ended) {
                deletion_value.deserialize(vbp, vsp);
                deletion_key.deserialize(kbp, ksp);
                free((char*) kbp);
                //free((char*) vbp);
            }
        }
        if (should_step_additions) {
            kbp = cursor_additions->get(&ksp, &vbp, &vsp, false);
            have_additions_ended = kbp == NULL;
            if (!have_additions_ended) {
                addition_value.deserialize(vbp, vsp);
                addition_key.deserialize(kbp, ksp);
                free((char*) kbp);
                //free((char*) vbp);
            }
        }

        if (has_patch_ended && have_deletions_ended && have_additions_ended) {
            break;
        }

        // P: currently inserting triple
        // D: current deletion triple
        // A: current addition triple

        // Advance the iterator with the smallest current triple.
        // TODO: make calc lazy for efficiency
        int comp_addition = tripleStore->get_spo_comparator()->compare(patch_element.get_triple(), addition_key);
        int comp_deletion = tripleStore->get_spo_comparator()->compare(patch_element.get_triple(), deletion_key);
        int comp_deletion_addition = tripleStore->get_spo_comparator()->compare(deletion_key, addition_key);
        // Convenience values for readability
        bool p_lt_d = !has_patch_ended && (have_deletions_ended || comp_deletion < 0);
        bool p_lt_a = !has_patch_ended && (have_additions_ended || comp_addition < 0);
        bool p_eq_d = !has_patch_ended && !have_deletions_ended && comp_deletion == 0;
        bool p_eq_a = !has_patch_ended && !have_additions_ended && comp_addition == 0;
        bool p_gt_d = !have_deletions_ended && (has_patch_ended || comp_deletion > 0);
        bool p_gt_a = !have_additions_ended && (has_patch_ended || comp_addition > 0);
        bool d_lt_a = !have_deletions_ended && (have_additions_ended || comp_deletion_addition < 0);
        bool a_lt_d = !have_additions_ended && (have_deletions_ended || comp_deletion_addition > 0);
        bool d_eq_a = !have_deletions_ended && !have_additions_ended && comp_deletion_addition == 0;

        if (p_gt_d && d_lt_a) { // D < P && D < A
            // D++
            should_step_patch = false;
            should_step_deletions = true;
            should_step_additions = false;

            if (deletion_value.get_patch(0).get_patch_id() <= patch_id) { // Skip this deletion if our current patch id lies before the first patch id for D
                // Add patch id with updated patch positions to current deletion triple
                PatchPositions patch_positions = deletion_value.is_local_change(patch_id) ?
                                                 PatchPositions() : Patch::positions(deletion_key, sp_, s_o, s__, _po, _p_, __o, ___);
                long patch_value_index;
                if ((patch_value_index = deletion_value.get_patchvalue_index(patch_id)) < 0
                    || deletion_value.get_patch(patch_value_index).get_patch_positions() != patch_positions) { // Don't re-insert when already present for this patch id, except when patch positions have changed
                    deletion_value.add(PatchTreeDeletionValueElement(patch_id, patch_positions));
                    tripleStore->insertDeletionSingle(&deletion_key, &deletion_value, cursor_deletions);
                }
            }
        } else if (p_gt_a && a_lt_d) { // A < P && A < D
            // A++
            should_step_patch = false;
            should_step_deletions = false;
            should_step_additions = true;

            // Add patch id to the current addition triple
            if (addition_value.get_patch_id_at(0) <= patch_id // Skip this addition if our current patch id lies before the first patch id for A
                && addition_value.get_patchvalue_index(patch_id) < 0) { // Don't re-insert when already present for this patch id
                addition_value.add(patch_id);
                tripleStore->insertAdditionSingle(&addition_key, &addition_value, cursor_additions);
            }
        } else if (p_lt_a && p_lt_d) { // P < A && P < D
            // P++
            should_step_patch = true;
            should_step_deletions = false;
            should_step_additions = false;

            // Insert triple from the patch in either addition or deletion tree
            if (patch_element.is_addition()) {
                tripleStore->insertAdditionSingle(&patch_element.get_triple(), patch_id, false, true);
            } else {
                PatchPositions patch_positions = Patch::positions(patch_element.get_triple(), sp_, s_o, s__, _po, _p_, __o, ___);
                tripleStore->insertDeletionSingle(&patch_element.get_triple(), patch_positions, patch_id, false, true);
            }
        } else if (p_eq_d && p_lt_a) { // P = D && P < A
            // local change P, D
            should_step_patch = true;
            should_step_deletions = true;
            should_step_additions = false;

            if (patch_element.is_addition()) {
                // Add addition as local change
                tripleStore->insertAdditionSingle(&patch_element.get_triple(), patch_id, true, true);
            } else {
                // Carry over previous deletion value
                bool was_local_change = deletion_value.is_local_change(patch_id);
                PatchPositions patch_positions = was_local_change ?
                                                 PatchPositions() : Patch::positions(patch_element.get_triple(), sp_, s_o, s__, _po, _p_, __o, ___);
                PatchTreeDeletionValueElement element = PatchTreeDeletionValueElement(patch_id, patch_positions);
                if (was_local_change) element.set_local_change();
                deletion_value.add(element);
                tripleStore->insertDeletionSingle(&deletion_key, &deletion_value, cursor_deletions);
            }
        } else if (p_eq_a && p_lt_d) { // P = A && P < D
            // local change P, A
            should_step_patch = true;
            should_step_deletions = false;
            should_step_additions = true;

            if (!patch_element.is_addition()) {
                // Add deletion as local change
                tripleStore->insertDeletionSingle(&patch_element.get_triple(), PatchPositions(), patch_id, true, true);
            } else {
                // Carry over previous addition value
                bool was_local_change = addition_value.is_local_change(patch_id);
                addition_value.add(patch_id);
                if (was_local_change) addition_value.set_local_change(patch_id);
                tripleStore->insertAdditionSingle(&addition_key, &addition_value);
            }
        } else { // (D = A) < P   or   P = A = D

            // carry over local change if < P, or invert existing local change if = P
            should_step_patch = !p_gt_d;
            should_step_deletions = true;
            should_step_additions = true;

            // Addition, deletion and patch element are equal.
            // That means that the local change must be inverted.
            // We only update the value with the smallest patch id, because we will never store +/- at the same time for the same patch id

            int largest_patch_id_addition = addition_value.get_patch_id_at(addition_value.get_size() - 1);
            int largest_patch_id_deletion = deletion_value.get_patch(deletion_value.get_size() - 1).get_patch_id();
            bool is_local_change = largest_patch_id_addition < largest_patch_id_deletion ? deletion_value.is_local_change(patch_id) : addition_value.is_local_change(patch_id);
            bool add_addition = largest_patch_id_addition > largest_patch_id_deletion;
            bool add_deletion = largest_patch_id_deletion > largest_patch_id_addition;
            if (should_step_patch) {
                is_local_change = !is_local_change;
                add_addition = !add_addition;
                add_deletion = !add_deletion;
            }

            if (add_addition) {
                addition_value.add(patch_id);
                if (is_local_change) addition_value.set_local_change(patch_id);
                tripleStore->insertAdditionSingle(&addition_key, &addition_value, cursor_additions);
            } else if (add_deletion) {
                PatchPositions patch_positions = is_local_change ?
                                                 PatchPositions() : Patch::positions(patch_element.get_triple(), sp_, s_o, s__, _po, _p_, __o, ___);
                PatchTreeDeletionValueElement deletion_value_element(patch_id, patch_positions);
                if (is_local_change) deletion_value_element.set_local_change();
                deletion_value.add(deletion_value_element);
                tripleStore->insertDeletionSingle(&deletion_key, &deletion_value, cursor_deletions);
            } else {
                cerr << "comp_deletion: " << comp_deletion << endl;
                cerr << "comp_addition: " << comp_addition << endl;
                cerr << "triple patch: " << patch_element.get_triple().to_string() << endl;
                cerr << "triple deletion: " << deletion_key.to_string() << endl;
                cerr << "triple addition: " << addition_key.to_string() << endl;
                throw std::invalid_argument(
                        "The store contains a locally changed addition and deletion for the same triple in the same patch id.");
            }
        }

        // Don't let iterators continue if they have ended
        if (should_step_deletions && have_deletions_ended) {
            should_step_deletions = false;
        }
        if (should_step_additions && have_additions_ended) {
            should_step_additions = false;
        }
        if (should_step_deletions) {
            cursor_deletions->step();
        }
        if (should_step_additions) {
            cursor_additions->step();
        }
    }

    NOTIFYMSG(progressListener, "\nFinished patch insertion\n");
    if (patch_id > max_patch_id) {
        max_patch_id = patch_id;
    }

    delete cursor_deletions;
    delete cursor_additions;

    deinit_temp_insertion_trees(sp_, s_o, s__, _po, _p_, __o);
}

bool PatchTree::append(PatchElementIterator* patch_it, int patch_id, ProgressListener* progressListener) {
    PatchElement element(Triple(0, 0, 0), true);
    // TODO: we can probably remove this, this shouldn't be a real problem. We should just crash when this occurs
    while (patch_it->next(&element)) {
        // We IGNORE the element type, because it makes no sense to have +/- for the same triple in the same patch!
        if(contains(element, patch_id, true)) {
            return false;
        }
    }
    patch_it->goToStart();
    append_unsafe(patch_it, patch_id, progressListener);
    return true;
}

bool PatchTree::append(const PatchSorted& patch, int patch_id, ProgressListener* progressListener) {
    PatchElementIteratorVector* it = new PatchElementIteratorVector(&patch.get_vector());
    bool ret = append(it, patch_id, progressListener);
    delete it;
    return ret;
}

bool PatchTree::contains(const PatchElement& patch_element, int patch_id, bool ignore_type) const {
    if (ignore_type) {
        return contains_addition(patch_element, patch_id) || contains_deletion(patch_element, patch_id);
    }
    if (patch_element.is_addition()) {
        return contains_addition(patch_element, patch_id);
    } else {
        return contains_deletion(patch_element, patch_id);
    }
}

bool PatchTree::contains_addition(const PatchElement& patch_element, int patch_id) const {
    PatchTreeKey key = patch_element.get_triple();
    size_t key_size, value_size;
    const char* raw_key = key.serialize(&key_size);
    const char* raw_value = tripleStore->getDefaultAdditionsTree()->get(raw_key, key_size, &value_size);

    // First, we check if the key is present
    bool ret = raw_value != NULL;
    if(ret) {
        // After that, we have to deserialize the value and check if it exists for the given patch.
        PatchTreeAdditionValue value;
        value.deserialize(raw_value, value_size);
        ret = value.is_patch_id(patch_id);
    }
    free((char*) raw_key);
    free((char*) raw_value);
    return ret;
}

bool PatchTree::contains_deletion(const PatchElement& patch_element, int patch_id) const {
    PatchTreeKey key = patch_element.get_triple();
    size_t key_size, value_size;
    const char* raw_key = key.serialize(&key_size);
    const char* raw_value = tripleStore->getDeletionsTree()->get(raw_key, key_size, &value_size);

    // First, we check if the key is present
    bool ret = raw_value != NULL;
    if(ret) {
        // After that, we have to deserialize the value and check if it exists for the given patch.
        PatchTreeDeletionValue value;
        value.deserialize(raw_value, value_size);
        long i = value.get_patchvalue_index(patch_id);
        ret = i >= 0;
    }
    free((char*) raw_key);
    free((char*) raw_value);
    return ret;
}

void PatchTree::reconstruct_to_patch(Patch* patch, int patch_id, bool ignore_local_changes) const {
    PatchTreeIterator it = iterator(patch_id, false);
    it.set_filter_local_changes(ignore_local_changes);
    PatchTreeKey key;
    PatchTreeValue value;
    while(it.next(&key, &value)) {
        PatchElement patchElement(key, value.is_addition(patch_id, false));
        // If the value does not exist yet for this patch id, go look at the previous patch id.
        patchElement.set_local_change(value.is_exact(patch_id) ? value.is_local_change(patch_id) : value.is_local_change(patch_id - 1));
        patch->add(patchElement);
    }
}

PatchHashed* PatchTree::reconstruct_patch_hashed(int patch_id, bool ignore_local_changes) const {
    PatchHashed* patch = new PatchHashed();
    reconstruct_to_patch(patch, patch_id, ignore_local_changes);
    return patch;
}

PatchSorted* PatchTree::reconstruct_patch(int patch_id, bool ignore_local_changes) const {
    PatchSorted* patch = new PatchSorted(get_element_comparator());
    reconstruct_to_patch(patch, patch_id, ignore_local_changes);
    return patch;
}

PatchTreeIterator PatchTree::iterator(PatchTreeKey* key) const {
    DB::Cursor* cursor_deletions = tripleStore->getDeletionsTree()->cursor();
    DB::Cursor* cursor_additions = tripleStore->getDefaultAdditionsTree()->cursor();
    size_t size;
    const char* data = key->serialize(&size);
    cursor_deletions->jump(data, size);
    cursor_additions->jump(data, size);
    free((char*) data);
    PatchTreeIterator patchTreeIterator(cursor_deletions, cursor_additions, get_spo_comparator());
    return patchTreeIterator;
}

PatchTreeIterator PatchTree::iterator(int patch_id, bool exact) const {
    DB::Cursor* cursor_deletions = tripleStore->getDeletionsTree()->cursor();
    DB::Cursor* cursor_additions = tripleStore->getDefaultAdditionsTree()->cursor();
    cursor_deletions->jump();
    cursor_additions->jump();
    PatchTreeIterator patchTreeIterator(cursor_deletions, cursor_additions, get_spo_comparator());
    patchTreeIterator.set_patch_filter(patch_id, exact);
    return patchTreeIterator;
}

PatchTreeIterator PatchTree::iterator(PatchTreeKey *key, int patch_id, bool exact) const {
    DB::Cursor* cursor_deletions = tripleStore->getDeletionsTree()->cursor();
    DB::Cursor* cursor_additions = tripleStore->getDefaultAdditionsTree()->cursor();
    size_t size;
    const char* data = key->serialize(&size);
    cursor_deletions->jump(data, size);
    cursor_additions->jump(data, size);
    free((char*) data);
    PatchTreeIterator patchTreeIterator(cursor_deletions, cursor_additions, get_spo_comparator());
    patchTreeIterator.set_patch_filter(patch_id, exact);
    return patchTreeIterator;
}

PatchTreeIterator* PatchTree::iterator(const Triple *triple_pattern) const {
    DB::Cursor* cursor_deletions = tripleStore->getDeletionsTree()->cursor();
    DB::Cursor* cursor_additions = tripleStore->getDefaultAdditionsTree()->cursor();
    size_t size;
    const char* data = triple_pattern->serialize(&size);
    cursor_deletions->jump(data, size);
    cursor_additions->jump(data, size);
    free((char*) data);
    PatchTreeIterator* patchTreeIterator = new PatchTreeIterator(cursor_deletions, cursor_additions, get_spo_comparator());
    patchTreeIterator->set_triple_pattern_filter(*triple_pattern);
    return patchTreeIterator;
}

std::pair<PatchPosition, Triple> PatchTree::deletion_count(const Triple &triple_pattern_jump, const Triple &triple_pattern_match, int patch_id, bool force_reverse) const {
    DB::Cursor* cursor_deletions = tripleStore->getDeletionsTree()->cursor();

    // Try jumping backwards to the position where the triple_pattern matches
    // If this jump succeeds, we will be _before_ the last matching triple, so we keep the normal iteration order.
    // If this jump failed, we jump to the very last record, which will be _after_ the last matching triple, so we reverse the iteration order.
    size_t size;
    const char* data = triple_pattern_jump.serialize(&size);
    bool hasJumped = cursor_deletions->jump_back(data, size);
    if (!hasJumped) {
        cursor_deletions->jump_back();
    }
    free((char*) data);

    PatchTreeIterator patchTreeIterator(cursor_deletions, NULL, get_spo_comparator());
    patchTreeIterator.set_patch_filter(patch_id, true);
    patchTreeIterator.set_triple_pattern_filter(triple_pattern_match);
    if (!hasJumped || force_reverse) patchTreeIterator.set_reverse(true);

    PatchTreeKey key;
    PatchTreeDeletionValue value;
    if(patchTreeIterator.next_deletion(&key, &value, true)) {
        return std::make_pair(value.get(patch_id).get_patch_positions().get_by_pattern(triple_pattern_match) + 1, key);
    }
    return std::make_pair((PatchPosition) 0, Triple());
};

std::pair<PatchPosition, Triple> PatchTree::deletion_count_until(const Triple &triple_pattern, int patch_id) const {
    return deletion_count(triple_pattern, triple_pattern, patch_id, false);
}

std::pair<PatchPosition, Triple> PatchTree::deletion_count_including(const Triple &triple_pattern, int patch_id) const {
    unsigned int max_id = (unsigned int) -1;
    return deletion_count(Triple(
            triple_pattern.get_subject() == 0 ? max_id : triple_pattern.get_subject(),
            triple_pattern.get_predicate() == 0 ? max_id : triple_pattern.get_predicate(),
            triple_pattern.get_object() == 0 ? max_id : triple_pattern.get_object()
    ), triple_pattern, patch_id, true);
}

PositionedTripleIterator* PatchTree::deletion_iterator_from(const Triple& offset, int patch_id, const Triple& triple_pattern) const {
    DB::Cursor* cursor_deletions = tripleStore->getDeletionsTree()->cursor();
    size_t size;
    const char* data = offset.serialize(&size);
    cursor_deletions->jump(data, size);
    free((char*) data);
    PatchTreeIterator* it = new PatchTreeIterator(cursor_deletions, NULL, get_spo_comparator());
    if (patch_id >= 0) it->set_patch_filter(patch_id, false);
    it->set_triple_pattern_filter(triple_pattern);
    it->set_filter_local_changes(true);
    return new PositionedTripleIterator(it, patch_id, triple_pattern);
}

PatchTreeDeletionValue* PatchTree::get_deletion_value(const Triple &triple) const {
    size_t ksp, vsp;
    const char* kbp = triple.serialize(&ksp);
    const char* vbp = tripleStore->getDeletionsTree()->get(kbp, ksp, &vsp);
    if (vbp != NULL) {
        PatchTreeDeletionValue* value = new PatchTreeDeletionValue();
        value->deserialize(vbp, vsp);
        return value;
    }
    return NULL;
}

PatchTreeDeletionValue* PatchTree::get_deletion_value_after(const Triple& triple_pattern) const {
    size_t ksp, vsp;
    const char *kbp = triple_pattern.serialize(&ksp);
    DB::Cursor* cursor = tripleStore->getDeletionsTree()->cursor();
    if (!cursor->jump(kbp, ksp)) {
        free((char*) kbp);
        return NULL;
    }
    const char *vbp;
    kbp = cursor->get(&ksp, &vbp, &vsp);
    if (vbp == NULL) {
        free((char*) kbp);
        return NULL;
    }

    kbp = cursor->get(&ksp, &vbp, &vsp);
    Triple triple;
    triple.deserialize(kbp, ksp);
    if (!Triple::pattern_match_triple(triple, triple_pattern)) {
        free((char*) kbp);
        return NULL;
    }

    PatchTreeDeletionValue* deletion_value = new PatchTreeDeletionValue();
    deletion_value->deserialize(vbp, vsp);
    free((char*) kbp);
    return deletion_value;
}

PatchPositions PatchTree::get_deletion_patch_positions(const Triple& triple, int patch_id, bool override___, PatchPosition ___) const {
    return PatchPositions(
            deletion_count_including(Triple(triple.get_subject(), triple.get_predicate(), 0), patch_id).first,
            deletion_count_including(Triple(triple.get_subject(), 0, triple.get_object()), patch_id).first,
            deletion_count_including(Triple(triple.get_subject(), 0, 0), patch_id).first,
            deletion_count_including(Triple(0, triple.get_predicate(), triple.get_object()), patch_id).first,
            deletion_count_including(Triple(0, triple.get_predicate(), 0), patch_id).first,
            deletion_count_including(Triple(0, 0, triple.get_object()), patch_id).first,
            override___ ? ___ : deletion_count_including(Triple(0, 0, 0), patch_id).first
    );
}

PatchTreeTripleIterator* PatchTree::addition_iterator_from(long offset, int patch_id, const Triple& triple_pattern) const {
    DB::Cursor* cursor = tripleStore->getTree(triple_pattern)->cursor();
    size_t size;
    const char* data = triple_pattern.serialize(&size);
    cursor->jump(data, size);
    PatchTreeIterator* it = new PatchTreeIterator(NULL, cursor, get_spo_comparator());
    it->set_patch_filter(patch_id, true);
    it->set_triple_pattern_filter(triple_pattern);
    it->set_filter_local_changes(true);
    // TODO: If this this ridiculous loop becomes too inefficient, make an offset map
    PatchTreeKey key;
    PatchTreeAdditionValue value;
    while(offset-- > 0 && it->next_addition(&key, &value));
    return new PatchTreeTripleIterator(it, triple_pattern);
}

PatchTreeIterator* PatchTree::addition_iterator(const Triple &triple_pattern) const {
    DB::Cursor* cursor = tripleStore->getTree(triple_pattern)->cursor();
    size_t size;
    const char* data = triple_pattern.serialize(&size);
    cursor->jump(data, size);
    PatchTreeIterator* it = new PatchTreeIterator(NULL, cursor, get_spo_comparator());
    it->set_triple_pattern_filter(triple_pattern);
    return it;
}

size_t PatchTree::addition_count(int patch_id, const Triple& triple_pattern) const {
    PatchTreeTripleIterator* it = addition_iterator_from(0, patch_id, triple_pattern);
    size_t count = 0;
    Triple* triple = new Triple();
    while (it->next(triple)) count++;
    delete triple;
    delete it;
    return count;
}

PatchTreeAdditionValue* PatchTree::get_addition_value(const Triple &triple) const {
    size_t ksp, vsp;
    const char* kbp = triple.serialize(&ksp);
    const char* vbp = tripleStore->getDefaultAdditionsTree()->get(kbp, ksp, &vsp);
    if (vbp != NULL) {
        PatchTreeAdditionValue* value = new PatchTreeAdditionValue();
        value->deserialize(vbp, vsp);
        return value;
    }
    return NULL;
}

PatchTreeKeyComparator* PatchTree::get_spo_comparator() const {
    return tripleStore->get_spo_comparator();
}

PatchElementComparator *PatchTree::get_element_comparator() const {
    return tripleStore->get_element_comparator();
}

const int PatchTree::get_max_patch_id() {
    return max_patch_id;
}

const int PatchTree::get_min_patch_id() {
    return min_patch_id;
}

void PatchTree::write_metadata() {
    ofstream metadata_file;
    metadata_file.open(metadata_filename);
    metadata_file << get_max_patch_id();
    metadata_file.close();
}

void PatchTree::read_metadata() {
    ifstream metadata_file;
    metadata_file.open(metadata_filename);
    if (metadata_file.good()) {
        string max_patch_id_str;
        metadata_file >> max_patch_id_str;
        max_patch_id = stoi(max_patch_id_str);
        metadata_file.close();
    }
}
