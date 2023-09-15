#include <kchashdb.h>

#include "patch_tree.h"
#include "../simpleprogresslistener.h"


PatchTree::PatchTree(string basePath, int min_patch_id, std::shared_ptr<DictionaryManager> dict, int8_t kc_opts, bool readonly)
        : metadata_filename(basePath + METADATA_FILENAME_BASE(min_patch_id)), min_patch_id(min_patch_id), max_patch_id(min_patch_id), readonly(readonly) {
    tripleStore = new TripleStore(basePath + PATCHTREE_FILENAME_BASE(min_patch_id), dict, kc_opts, readonly);
    read_metadata();

    if (!readonly) {
        std::remove(".additions.sp_.tmp");
        std::remove(".additions.s_o.tmp");
        std::remove(".additions.s__.tmp");
        std::remove(".additions._po.tmp");
        std::remove(".additions._p_.tmp");
        std::remove(".additions.__o.tmp");

        sp_.open(".additions.sp_.tmp", kyotocabinet::HashDB::OWRITER | kyotocabinet::HashDB::OCREATE);
        s_o.open(".additions.s_o.tmp", kyotocabinet::HashDB::OWRITER | kyotocabinet::HashDB::OCREATE);
        s__.open(".additions.s__.tmp", kyotocabinet::HashDB::OWRITER | kyotocabinet::HashDB::OCREATE);
        _po.open(".additions._po.tmp", kyotocabinet::HashDB::OWRITER | kyotocabinet::HashDB::OCREATE);
        _p_.open(".additions._p_.tmp", kyotocabinet::HashDB::OWRITER | kyotocabinet::HashDB::OCREATE);
        __o.open(".additions.__o.tmp", kyotocabinet::HashDB::OWRITER | kyotocabinet::HashDB::OCREATE);
    }
};

PatchTree::~PatchTree() {
    if (!readonly) {
        write_metadata();
    }
    delete tripleStore;

    if (!readonly) {
        std::remove(".additions.sp_.tmp");
        std::remove(".additions.s_o.tmp");
        std::remove(".additions.s__.tmp");
        std::remove(".additions._po.tmp");
        std::remove(".additions._p_.tmp");
        std::remove(".additions.__o.tmp");
    }
}

void PatchTree::clear_temp_insertion_trees() {
    sp_.clear();
    s_o.clear();
    s__.clear();
    _po.clear();
    _p_.clear();
    __o.clear();
}

/*
 * --- START OF COMPARISON MACROS  ---
 * Convenience macro's and inline functions to improve readability of triple component comparison
 * This will ensure lazy calculation of potentially expensive comparisons.
 */

inline int comp_addition(bool& done_flag, TripleStore* tripleStore, PatchElement& patch_element, PatchTreeKey& deletion_key, PatchTreeKey& addition_key) {
    done_flag = true;
    return tripleStore->get_spo_comparator()->compare(patch_element.get_triple(), addition_key);
}

inline int comp_deletion(bool& done_flag, TripleStore* tripleStore, PatchElement& patch_element, PatchTreeKey& deletion_key, PatchTreeKey& addition_key) {
    done_flag = true;
    return tripleStore->get_spo_comparator()->compare(patch_element.get_triple(), deletion_key);
}

inline int comp_deletion_addition(bool& done_flag, TripleStore* tripleStore, PatchElement& patch_element, PatchTreeKey& deletion_key, PatchTreeKey& addition_key) {
    done_flag = true;
    return tripleStore->get_spo_comparator()->compare(deletion_key, addition_key);
}

#define PINS_COMP_PARAMS_DEF TripleStore* tripleStore, PatchElement& patch_element, PatchTreeKey& deletion_key, PatchTreeKey& addition_key, bool has_patch_ended, bool& has_comp_deletion_addition, int& comp_deletion_addition_v, bool have_deletions_ended, bool& has_comp_deletion, int& comp_deletion_v, bool have_additions_ended, bool& has_comp_addition, int& comp_addition_v
#define COMP_DELETION (!has_comp_deletion ? (comp_deletion_v = comp_deletion(has_comp_deletion, tripleStore, patch_element, deletion_key, addition_key)) : comp_deletion_v)
#define COMP_ADDITION (!has_comp_addition ? (comp_addition_v = comp_addition(has_comp_addition, tripleStore, patch_element, deletion_key, addition_key)) : comp_addition_v)
#define COMP_DELETION_ADDITION (!has_comp_deletion_addition ? (comp_deletion_addition_v = comp_deletion_addition(has_comp_deletion_addition, tripleStore, patch_element, deletion_key, addition_key)) : comp_deletion_addition_v)
inline bool p_lt_d(PINS_COMP_PARAMS_DEF) { return !has_patch_ended && (have_deletions_ended      || COMP_DELETION          < 0); }
inline bool p_lt_a(PINS_COMP_PARAMS_DEF) { return !has_patch_ended && (have_additions_ended      || COMP_ADDITION          < 0); }
inline bool p_eq_d(PINS_COMP_PARAMS_DEF) { return !has_patch_ended && !have_deletions_ended      && COMP_DELETION          == 0; }
inline bool p_eq_a(PINS_COMP_PARAMS_DEF) { return !has_patch_ended && !have_additions_ended      && COMP_ADDITION          == 0; }
inline bool p_gt_d(PINS_COMP_PARAMS_DEF) { return !have_deletions_ended && (has_patch_ended      || COMP_DELETION          > 0); }
inline bool p_gt_a(PINS_COMP_PARAMS_DEF) { return !have_additions_ended && (has_patch_ended      || COMP_ADDITION          > 0); }
inline bool d_lt_a(PINS_COMP_PARAMS_DEF) { return !have_deletions_ended && (have_additions_ended || COMP_DELETION_ADDITION < 0); }
inline bool a_lt_d(PINS_COMP_PARAMS_DEF) { return !have_additions_ended && (have_deletions_ended || COMP_DELETION_ADDITION > 0); }
#define PINS_COMP_PARAMS tripleStore, patch_element, deletion_key, addition_key, has_patch_ended, has_comp_deletion_addition, comp_deletion_addition_v, have_deletions_ended, has_comp_deletion, comp_deletion_v, have_additions_ended, has_comp_addition, comp_addition_v
#define P_LT_D p_lt_d(PINS_COMP_PARAMS)
#define P_LT_A p_lt_a(PINS_COMP_PARAMS)
#define P_EQ_D p_eq_d(PINS_COMP_PARAMS)
#define P_EQ_A p_eq_a(PINS_COMP_PARAMS)
#define P_GT_D p_gt_d(PINS_COMP_PARAMS)
#define P_GT_A p_gt_a(PINS_COMP_PARAMS)
#define D_LT_A d_lt_a(PINS_COMP_PARAMS)
#define A_LT_D a_lt_d(PINS_COMP_PARAMS)

/*
 * --- END OF COMPARISON MACROS  ---
 */

void PatchTree::append_unsafe(PatchElementIterator* patch_it, int patch_id, hdt::ProgressListener *progressListener) {
    if (readonly) {
        throw std::invalid_argument("Can not append in read-only mode");
    }

    // TODO: enable this for improved efficiency, and after it has been fixed...
    //PatchElementIteratorBuffered* patch_it = new PatchElementIteratorBuffered(patch_it_original, PATCH_INSERT_BUFFER_SIZE);

    const char *kbp, *vbp;
    size_t ksp, vsp;

#ifdef COMPRESSED_DEL_VALUES
    PatchTreeDeletionValue deletion_value(max_patch_id);
#else
    PatchTreeDeletionValue deletion_value;
#endif
#ifdef COMPRESSED_ADD_VALUES
    PatchTreeAdditionValue addition_value(max_patch_id);
#else
    PatchTreeAdditionValue addition_value;
#endif
    PatchTreeKey deletion_key, addition_key;
    PatchElement patch_element(Triple(0, 0, 0), true);

    // Loop over SPO deletion and addition trees
    // We do this together to be able to efficiently determine the local change flags
    NOTIFYMSG(progressListener, "Inserting into deletion and addition trees...\n");
    kyotocabinet::DB::Cursor* cursor_deletions = tripleStore->getDefaultDeletionsTree()->cursor();
    kyotocabinet::DB::Cursor* cursor_additions = tripleStore->getDefaultAdditionsTree()->cursor();

    cursor_deletions->jump();
    cursor_additions->jump();

    // Counters for all possible patch positions
    // We use KC hashmaps to store the potentially large amounts of triple patterns to avoid running out of memory.
    PatchPosition ___ = 0;
    clear_temp_insertion_trees();

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
            have_deletions_ended = kbp == nullptr;
            if (!have_deletions_ended) {
                deletion_value.deserialize(vbp, vsp);
                deletion_key.deserialize(kbp, ksp);
                delete[] kbp;
            }
        }
        if (should_step_additions) {
            kbp = cursor_additions->get(&ksp, &vbp, &vsp, false);
            have_additions_ended = kbp == nullptr;
            if (!have_additions_ended) {
                addition_key.deserialize(kbp, ksp);
                addition_value.deserialize(vbp, vsp);
                delete[] kbp;
            }
        }

        if (has_patch_ended && have_deletions_ended && have_additions_ended) {
            break;
        }

        // P: currently inserting triple
        // D: current deletion triple
        // A: current addition triple

        // Advance the iterator with the smallest current triple.
        bool has_comp_deletion_addition = false;
        bool has_comp_deletion = false;
        bool has_comp_addition = false;
        int comp_deletion_addition_v, comp_deletion_v, comp_addition_v;
        if (P_GT_D && D_LT_A) { // D < P && D < A
            // D++
            should_step_patch = false;
            should_step_deletions = true;
            should_step_additions = false;

            if (deletion_value.get_patch_at(0).get_patch_id() <= patch_id) { // Skip this deletion if our current patch id lies before the first patch id for D
                // Add patch id with updated patch positions to current deletion triple
                PatchPositions patch_positions = deletion_value.is_local_change(patch_id) ?
                                                 PatchPositions() : Patch::positions(deletion_key, sp_, s_o, s__, _po, _p_, __o, ___);
                long patch_value_index;
                if ((patch_value_index = deletion_value.get_patchvalue_index(patch_id)) < 0
                    || deletion_value.get_patch_at(patch_value_index).get_patch_positions() != patch_positions) { // Don't re-insert when already present for this patch id, except when patch positions have changed
#ifdef COMPRESSED_DEL_VALUES
                    bool has_changed = deletion_value.add(PatchTreeDeletionValueElement(patch_id, patch_positions));
                    PatchTreeDeletionValueReduced deletion_value_reduced = deletion_value.to_reduced();
                    if (has_changed) {
                        tripleStore->insertDeletionSingle(&deletion_key, &deletion_value, &deletion_value_reduced, cursor_deletions);
                    }
#else
                    deletion_value.add(PatchTreeDeletionValueElement(patch_id, patch_positions));
                    PatchTreeDeletionValueReduced deletion_value_reduced = deletion_value.to_reduced();
                    tripleStore->insertDeletionSingle(&deletion_key, &deletion_value, &deletion_value_reduced, cursor_deletions);
#endif
                }
            }
        } else if (P_GT_A && A_LT_D) { // A < P && A < D
            // A++
            should_step_patch = false;
            should_step_deletions = false;
            should_step_additions = true;

            // Add patch id to the current addition triple
            if (addition_value.get_patch_id_at(0) <= patch_id // Skip this addition if our current patch id lies before the first patch id for A
                && !addition_value.is_patch_id(patch_id)) { // Don't re-insert when already present for this patch id
#ifdef COMPRESSED_ADD_VALUES
                bool has_changed = addition_value.add(patch_id);
                if (has_changed) {
                    tripleStore->insertAdditionSingle(&addition_key, &addition_value, cursor_additions);
                }
#else
                addition_value.add(patch_id);
                tripleStore->insertAdditionSingle(&addition_key, &addition_value, cursor_additions);
#endif
            }
            if (!addition_value.is_local_change(patch_id)) {
                tripleStore->increment_addition_counts(patch_id, addition_key);
            }
            if (!addition_value.is_local_change(addition_value.get_patch_id_at(0))) {
                tripleStore->increment_addition_counts(0, addition_key);
            }
        } else if (P_LT_A && P_LT_D) { // P < A && P < D
            // P++
            should_step_patch = true;
            should_step_deletions = false;
            should_step_additions = false;

            // Insert triple from the patch in either addition or deletion tree
            if (patch_element.is_addition()) {
                tripleStore->insertAdditionSingle(&patch_element.get_triple(), patch_id, false, true);
                tripleStore->increment_addition_counts(0, patch_element.get_triple());
            } else {
                PatchPositions patch_positions = Patch::positions(patch_element.get_triple(), sp_, s_o, s__, _po, _p_, __o, ___);
                tripleStore->insertDeletionSingle(&patch_element.get_triple(), patch_positions, patch_id, false, true);
            }
        } else if (P_EQ_D && P_LT_A) { // P = D && P < A
            // local change P, D
            should_step_patch = true;
            should_step_deletions = true;
            should_step_additions = false;

            if (patch_element.is_addition()) {
                // Add addition as local change
#ifdef COMPRESSED_DEL_VALUES
                bool has_changed = deletion_value.del(patch_id);
                if (has_changed) {
                    PatchTreeDeletionValueReduced deletion_value_reduced = deletion_value.to_reduced();
                    tripleStore->insertDeletionSingle(&deletion_key, &deletion_value, &deletion_value_reduced, cursor_deletions);
                }
#endif
                tripleStore->insertAdditionSingle(&patch_element.get_triple(), patch_id, true, true);
            } else {
                // Carry over previous deletion value
#ifdef COMPRESSED_DEL_VALUES
                bool was_local_change = deletion_value.is_local_change(patch_id);
                PatchPositions patch_positions = was_local_change ?
                                                 PatchPositions() : Patch::positions(patch_element.get_triple(), sp_, s_o, s__, _po, _p_, __o, ___);
                PatchTreeDeletionValueElement element = PatchTreeDeletionValueElement(patch_id, patch_positions);
                if (was_local_change) element.set_local_change();
                bool has_changed = deletion_value.add(element);
                if (has_changed) {
                    PatchTreeDeletionValueReduced deletion_value_reduced = deletion_value.to_reduced();
                    tripleStore->insertDeletionSingle(&deletion_key, &deletion_value, &deletion_value_reduced, cursor_deletions);
                }
#else
                bool was_local_change = deletion_value.is_local_change(patch_id);
                PatchPositions patch_positions = was_local_change ?
                                                 PatchPositions() : Patch::positions(patch_element.get_triple(), sp_, s_o, s__, _po, _p_, __o, ___);
                PatchTreeDeletionValueElement element = PatchTreeDeletionValueElement(patch_id, patch_positions);
                if (was_local_change) element.set_local_change();
                deletion_value.add(element);
                PatchTreeDeletionValueReduced deletion_value_reduced = deletion_value.to_reduced();
                tripleStore->insertDeletionSingle(&deletion_key, &deletion_value, &deletion_value_reduced, cursor_deletions);
#endif
            }
        } else if (P_EQ_A && P_LT_D) { // P = A && P < D
            // local change P, A
            should_step_patch = true;
            should_step_deletions = false;
            should_step_additions = true;

            if (!patch_element.is_addition()) {
                // Add deletion as local change
#ifdef COMPRESSED_ADD_VALUES
                bool has_changed = addition_value.del(patch_id);
                has_changed |= addition_value.unset_local_change(patch_id);
                if (has_changed) {
                    tripleStore->insertAdditionSingle(&addition_key, &addition_value, cursor_additions);
                }
#endif
                tripleStore->insertDeletionSingle(&patch_element.get_triple(), PatchPositions(), patch_id, true, true);
            } else {
                // Carry over previous addition value
#ifndef COMPRESSED_ADD_VALUES
                bool was_local_change = addition_value.is_local_change(patch_id);
                addition_value.add(patch_id);
                if (was_local_change) addition_value.set_local_change(patch_id);
                tripleStore->insertAdditionSingle(&addition_key, &addition_value);
                if (!was_local_change) {
                    tripleStore->increment_addition_counts(patch_id, addition_key);
                }
#else
                bool has_changed = addition_value.add(patch_id);
                if (has_changed)
                    tripleStore->insertAdditionSingle(&addition_key, &addition_value, cursor_additions);
                if (!addition_value.is_local_change(patch_id)) {
                    tripleStore->increment_addition_counts(patch_id, addition_key);
                }
#endif
            }
            if (!addition_value.is_local_change(addition_value.get_patch_id_at(0))) {
                tripleStore->increment_addition_counts(0, addition_key);
            }
        } else { // (D = A) < P   or   P = A = D
            // carry over local change if < P, or invert existing local change if = P
            should_step_patch = !P_GT_D;
            should_step_deletions = true;
            should_step_additions = true;

            // Addition, deletion and patch element are equal.
            // That means that the local change must be inverted.
            // We only update the value with the smallest patch id, because we will never store +/- at the same time for the same patch id

#ifdef COMPRESSED_ADD_VALUES
            addition_value.set_max_patch_id(patch_id);
#endif
            int largest_patch_id_addition = addition_value.get_patch_id_at(addition_value.get_size() - 1);
            int largest_patch_id_deletion = deletion_value.get_patch_at(deletion_value.get_size() - 1).get_patch_id();
            bool is_local_change = largest_patch_id_addition < largest_patch_id_deletion ? deletion_value.is_local_change(patch_id) : addition_value.is_local_change(patch_id);
            bool add_addition = largest_patch_id_addition > largest_patch_id_deletion;
            bool add_deletion = largest_patch_id_deletion > largest_patch_id_addition;
            if (should_step_patch) {
                is_local_change = !is_local_change;
                add_addition = !add_addition;
                add_deletion = !add_deletion;
            }
#ifdef COMPRESSED_ADD_VALUES
            addition_value.set_max_patch_id(patch_id+1);
#endif

            // Sanity check, disabled for efficiency
            /*
            if (COMP_DELETION || COMP_ADDITION) {
                cerr << "largest_patch_id_addition: " << largest_patch_id_addition << endl;
                cerr << "largest_patch_id_deletion: " << largest_patch_id_deletion << endl;
                cerr << "is_local_change: " << is_local_change << endl;
                cerr << "should_step_patch: " << should_step_patch << endl;
                cerr << "comp_deletion: " << COMP_DELETION << endl;
                cerr << "comp_addition: " << COMP_ADDITION << endl;
                cerr << "comp_d_lt_a: " << D_LT_A << endl;
                cerr << "comp_a_lt_d: " << A_LT_D << endl;
                cerr << "triple patch: " << patch_element.get_triple().to_string() << endl;
                cerr << "triple deletion: " << deletion_key.to_string() << endl;
                cerr << "triple addition: " << addition_key.to_string() << endl;
                throw std::invalid_argument("D is not equal to A.");
            }*/

            if (!addition_value.is_local_change(addition_value.get_patch_id_at(0))) {
                tripleStore->increment_addition_counts(0, addition_key);
            }
            if (add_addition) {
#ifdef COMPRESSED_ADD_VALUES
                bool has_changed_add = addition_value.add(patch_id);
                if (is_local_change)
                    has_changed_add |= addition_value.set_local_change(patch_id);
                if (has_changed_add)
                    tripleStore->insertAdditionSingle(&addition_key, &addition_value, cursor_additions);
                if (!is_local_change) {
                    tripleStore->increment_addition_counts(patch_id, addition_key);
                }
#else
                addition_value.add(patch_id);
                if (is_local_change) addition_value.set_local_change(patch_id);
                tripleStore->insertAdditionSingle(&addition_key, &addition_value, cursor_additions);
                if (!is_local_change) {
                    tripleStore->increment_addition_counts(patch_id, addition_key);
                }
#endif
#ifdef COMPRESSED_DEL_VALUES
                bool has_changed_del = deletion_value.del(patch_id);
                if (has_changed_del) {
                    PatchTreeDeletionValueReduced deletion_value_reduced = deletion_value.to_reduced();
                    tripleStore->insertDeletionSingle(&deletion_key, &deletion_value, &deletion_value_reduced, cursor_deletions);
                }
#endif
            } else if (add_deletion) {
#ifdef COMPRESSED_DEL_VALUES
                PatchPositions patch_positions = is_local_change ?
                                                 PatchPositions() : Patch::positions(patch_element.get_triple(), sp_, s_o, s__, _po, _p_, __o, ___);
                PatchTreeDeletionValueElement deletion_value_element(patch_id, patch_positions);
                if (is_local_change) deletion_value_element.set_local_change();
                bool has_changed_del = deletion_value.add(deletion_value_element);
                if (has_changed_del) {
                    PatchTreeDeletionValueReduced deletion_value_reduced = deletion_value.to_reduced();
                    tripleStore->insertDeletionSingle(&deletion_key, &deletion_value, &deletion_value_reduced, cursor_deletions);
                }
#else
                PatchPositions patch_positions = is_local_change ?
                                                 PatchPositions() : Patch::positions(patch_element.get_triple(), sp_, s_o, s__, _po, _p_, __o, ___);
                PatchTreeDeletionValueElement deletion_value_element(patch_id, patch_positions);
                if (is_local_change) deletion_value_element.set_local_change();
                deletion_value.add(deletion_value_element);
                PatchTreeDeletionValueReduced deletion_value_reduced = deletion_value.to_reduced();
                tripleStore->insertDeletionSingle(&deletion_key, &deletion_value, &deletion_value_reduced, cursor_deletions);
#endif
#ifdef COMPRESSED_ADD_VALUES
                bool has_changed_add = addition_value.del(patch_id);
                if (addition_value.is_local_change(patch_id))
                    has_changed_add |= addition_value.unset_local_change(patch_id);
                if (has_changed_add)
                    tripleStore->insertAdditionSingle(&addition_key, &addition_value, cursor_additions);
#endif
            } else {
                // TODO: enable me after fixing https://git.datasciencelab.ugent.be/linked-data-fragments/Patch-Store/issues/5
                // TODO: Temporarily disabled for reducing logging output
                /*cerr << "largest_patch_id_addition: " << largest_patch_id_addition << endl;
                cerr << "largest_patch_id_deletion: " << largest_patch_id_deletion << endl;
                cerr << "is_local_change: " << is_local_change << endl;
                cerr << "should_step_patch: " << should_step_patch << endl;
                cerr << "comp_deletion: " << COMP_DELETION << endl;
                cerr << "comp_addition: " << COMP_ADDITION << endl;
                cerr << "comp_d_lt_a: " << D_LT_A << endl;
                cerr << "comp_a_lt_d: " << A_LT_D << endl;
                cerr << "triple patch: " << patch_element.get_triple().to_string() << endl;
                cerr << "triple deletion: " << deletion_key.to_string() << endl;
                cerr << "triple addition: " << addition_key.to_string() << endl;
                throw std::invalid_argument(
                        "The store contains a locally changed addition and deletion for the same triple in the same patch id.");*/
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

    NOTIFYMSG(progressListener, "\nFlushing addition counts...\n");
    long addition_counts = tripleStore->flush_addition_counts();
    NOTIFYMSG(progressListener, ("\nSaved " + std::to_string(addition_counts) + " addition counts\n").c_str());

    NOTIFYMSG(progressListener, "\nFinished patch insertion\n");
    if (patch_id > max_patch_id) {
        max_patch_id = patch_id;
    }

    delete cursor_deletions;
    delete cursor_additions;
}

bool PatchTree::append(PatchElementIterator* patch_it, int patch_id, hdt::ProgressListener* progressListener) {
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

bool PatchTree::append(const PatchSorted& patch, int patch_id, hdt::ProgressListener* progressListener) {
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
    delete[] raw_key;

    // First, we check if the key is present
    bool ret = raw_value != nullptr;
    if(ret) {
        // After that, we have to deserialize the value and check if it exists for the given patch.
#ifdef COMPRESSED_ADD_VALUES
        PatchTreeAdditionValue value(max_patch_id);
#else
        PatchTreeAdditionValue value;
#endif
        value.deserialize(raw_value, value_size);
        delete[] raw_value;
        ret = value.is_patch_id(patch_id);
    }
    return ret;
}

bool PatchTree::contains_deletion(const PatchElement& patch_element, int patch_id) const {
    PatchTreeKey key = patch_element.get_triple();
    size_t key_size, value_size;
    const char* raw_key = key.serialize(&key_size);
    const char* raw_value = tripleStore->getDefaultDeletionsTree()->get(raw_key, key_size, &value_size);
    delete[] raw_key;

    // First, we check if the key is present
    bool ret = raw_value != nullptr;
    if(ret) {
        // After that, we have to deserialize the value and check if it exists for the given patch.
#ifdef COMPRESSED_DEL_VALUES
        PatchTreeDeletionValue value(max_patch_id);
#else
        PatchTreeDeletionValue value;
#endif
        value.deserialize(raw_value, value_size);
        delete[] raw_value;
        long i = value.get_patchvalue_index(patch_id);
        ret = i >= 0;
    }
    return ret;
}

void PatchTree::reconstruct_to_patch(Patch* patch, int patch_id, bool ignore_local_changes) const {
    PatchTreeIterator it = iterator(patch_id, false);
    it.set_filter_local_changes(ignore_local_changes);
    PatchTreeKey key;
#if defined(COMPRESSED_ADD_VALUES) || defined(COMPRESSED_DEL_VALUES)
    PatchTreeValue value(max_patch_id);
#else
    PatchTreeValue value;
#endif
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
    kyotocabinet::DB::Cursor* cursor_deletions = tripleStore->getDefaultDeletionsTree()->cursor();
    kyotocabinet::DB::Cursor* cursor_additions = tripleStore->getDefaultAdditionsTree()->cursor();
    size_t size;
    const char* data = key->serialize(&size);
    cursor_deletions->jump(data, size);
    cursor_additions->jump(data, size);
    delete[] data;
    PatchTreeIterator patchTreeIterator(cursor_deletions, cursor_additions, get_spo_comparator());
    return patchTreeIterator;
}

PatchTreeIterator PatchTree::iterator(int patch_id, bool exact) const {
    kyotocabinet::DB::Cursor* cursor_deletions = tripleStore->getDefaultDeletionsTree()->cursor();
    kyotocabinet::DB::Cursor* cursor_additions = tripleStore->getDefaultAdditionsTree()->cursor();
    cursor_deletions->jump();
    cursor_additions->jump();
    PatchTreeIterator patchTreeIterator(cursor_deletions, cursor_additions, get_spo_comparator());
    patchTreeIterator.set_patch_filter(patch_id, exact);
    return patchTreeIterator;
}

PatchTreeIterator PatchTree::iterator(PatchTreeKey *key, int patch_id, bool exact) const {
    kyotocabinet::DB::Cursor* cursor_deletions = tripleStore->getDefaultDeletionsTree()->cursor();
    kyotocabinet::DB::Cursor* cursor_additions = tripleStore->getDefaultAdditionsTree()->cursor();
    size_t size;
    const char* data = key->serialize(&size);
    cursor_deletions->jump(data, size);
    cursor_additions->jump(data, size);
    delete[] data;
    PatchTreeIterator patchTreeIterator(cursor_deletions, cursor_additions, get_spo_comparator());
    patchTreeIterator.set_patch_filter(patch_id, exact);
    return patchTreeIterator;
}

template <class DV>
PatchTreeIteratorBase<DV>* PatchTree::iterator(const Triple *triple_pattern) const {
    kyotocabinet::DB::Cursor* cursor_deletions = tripleStore->getDeletionsTree(*triple_pattern)->cursor();
    kyotocabinet::DB::Cursor* cursor_additions = tripleStore->getAdditionsTree(*triple_pattern)->cursor();
    size_t size;
    const char* data = triple_pattern->serialize(&size);
    cursor_deletions->jump(data, size);
    cursor_additions->jump(data, size);
    delete[] data;
    PatchTreeIteratorBase<DV>* patchTreeIterator = new PatchTreeIteratorBase<DV>(cursor_deletions, cursor_additions, get_spo_comparator());
    patchTreeIterator->set_triple_pattern_filter(*triple_pattern);
    return patchTreeIterator;
}

template <class DV>
std::pair<DV*, Triple> PatchTree::last_deletion_value(const Triple &triple_pattern, int patch_id) const {
    size_t max_id = (size_t) -1;
    Triple triple_pattern_jump(
            triple_pattern.get_subject() == 0 ? max_id : triple_pattern.get_subject(),
            triple_pattern.get_predicate() == 0 ? max_id : triple_pattern.get_predicate(),
            triple_pattern.get_object() == 0 ? max_id : triple_pattern.get_object()
    );
    kyotocabinet::DB::Cursor* cursor_deletions = tripleStore->getDeletionsTree(triple_pattern)->cursor();

    // Try jumping backwards to the position where the triple_pattern matches
    size_t size;
    const char* data = triple_pattern_jump.serialize(&size);
    bool hasJumped = cursor_deletions->jump_back(data, size);
    if (!hasJumped) {
        // A failure to jump means that there is no triple in the tree that matches the pattern, so we return count 0.
        delete[] data;
        delete cursor_deletions;
        return std::make_pair(nullptr, Triple());
    }
    delete[] data;

    PatchTreeIteratorBase<DV> patchTreeIterator(cursor_deletions, NULL, get_spo_comparator());
    patchTreeIterator.set_patch_filter(patch_id, true);
    patchTreeIterator.set_triple_pattern_filter(triple_pattern);
    patchTreeIterator.set_filter_local_changes(true);
    patchTreeIterator.set_reverse(true); // Because we start _after_ the last matching triple because of the jump_back.

    PatchTreeKey key;
#ifdef COMPRESSED_DEL_VALUES
    DV* value = new DV(max_patch_id);
#else
    DV* value = new DV();
#endif
    if(patchTreeIterator.next_deletion(&key, value, true)) {
        return std::make_pair(value, key);
    }
    delete value;
    return std::make_pair(nullptr, Triple());
}

std::pair<PatchPosition, Triple> PatchTree::deletion_count(const Triple &triple_pattern, int patch_id) const {
    PatchPosition patch_position;
    Triple triple;
    if (TripleStore::is_default_tree(triple_pattern)) {
        // If we are using the SPO-tree, patch positions are stored in there,
        // so we can immediately retrieve those and return them.
        std::pair<PatchTreeDeletionValue*, Triple> value = last_deletion_value<PatchTreeDeletionValue>(triple_pattern, patch_id);
        if (value.first == nullptr) {
            return std::make_pair((PatchPosition) 0, Triple());
        }
        patch_position = value.first->get(patch_id).get_patch_positions().get_by_pattern(triple_pattern) + 1;
        delete value.first;
        triple = value.second;
    } else {
        // If we are using a non-SPO-tree, we still know the exact last triple,
        // so we take that triple, and search for it in the SPO-tree, and retrieve the value there, which will be fast.
        std::pair<PatchTreeDeletionValueReduced*, Triple> value = last_deletion_value<PatchTreeDeletionValueReduced>(triple_pattern, patch_id);
        if (value.first == nullptr) {
            return std::make_pair((PatchPosition) 0, Triple());
        }
        delete value.first;
        PatchTreeDeletionValue *dv = get_deletion_value(value.second);
        patch_position = dv->get(patch_id).get_patch_positions().get_by_pattern(triple_pattern) + 1;
        triple = value.second;
        delete dv;
    }
    return std::make_pair(patch_position, triple);
}

PositionedTripleIterator* PatchTree::deletion_iterator_from(const Triple& offset, int patch_id, const Triple& triple_pattern) const {
    kyotocabinet::DB::Cursor* cursor_deletions = tripleStore->getDefaultDeletionsTree()->cursor();
    size_t size;
    const char* data = offset.serialize(&size);
    cursor_deletions->jump(data, size);
    delete[] data;
    PatchTreeIterator* it = new PatchTreeIterator(cursor_deletions, nullptr, get_spo_comparator());
    if (patch_id >= 0) it->set_patch_filter(patch_id, true);
    it->set_triple_pattern_filter(triple_pattern);
    it->set_filter_local_changes(true);
    it->set_early_break(false);
    return new PositionedTripleIterator(it, patch_id, triple_pattern);
}

PatchTreeDeletionValue* PatchTree::get_deletion_value(const Triple &triple) const {
    size_t ksp, vsp;
    const char* kbp = triple.serialize(&ksp);
    const char* vbp = tripleStore->getDefaultDeletionsTree()->get(kbp, ksp, &vsp);
    delete[] kbp;
    if (vbp != nullptr) {
#ifdef COMPRESSED_DEL_VALUES
        PatchTreeDeletionValue* value = new PatchTreeDeletionValue(max_patch_id);
#else
        PatchTreeDeletionValue* value = new PatchTreeDeletionValue();
#endif
        value->deserialize(vbp, vsp);
        delete[] vbp;
        return value;
    }
    return nullptr;
}

template <class DV>
PatchTreeDeletionValueBase<DV>* PatchTree::get_deletion_value_after(const Triple& triple_pattern) const {
    size_t ksp, vsp;
    const char *kbp = triple_pattern.serialize(&ksp);
    kyotocabinet::DB::Cursor* cursor = tripleStore->getDeletionsTree(triple_pattern)->cursor();
    if (!cursor->jump(kbp, ksp)) {
        free((char*) kbp);
        return nullptr;
    }
    const char *vbp;
    kbp = cursor->get(&ksp, &vbp, &vsp);
    if (vbp == nullptr) {
        delete[] kbp;
        return nullptr;
    }

    kbp = cursor->get(&ksp, &vbp, &vsp);
    Triple triple;
    triple.deserialize(kbp, ksp);
    if (!Triple::pattern_match_triple(triple, triple_pattern)) {
        delete[] kbp;
        return nullptr;
    }

    PatchTreeDeletionValueBase<DV>* deletion_value = nullptr;
    if (TripleStore::is_default_tree(triple_pattern)) {
#ifdef COMPRESSED_DEL_VALUES
        deletion_value = reinterpret_cast<PatchTreeDeletionValueBase<DV>*>(new PatchTreeDeletionValue(max_patch_id));
#else
        deletion_value = reinterpret_cast<PatchTreeDeletionValueBase<DV>*>(new PatchTreeDeletionValue());
#endif
    } else {
#ifdef COMPRESSED_DEL_VALUES
        deletion_value = reinterpret_cast<PatchTreeDeletionValueBase<DV>*>(new PatchTreeDeletionValueReduced(max_patch_id));
#else
        deletion_value = reinterpret_cast<PatchTreeDeletionValueBase<DV>*>(new PatchTreeDeletionValueReduced());
#endif
    }
    deletion_value->deserialize(vbp, vsp);
    delete[] kbp;
    return deletion_value;
}

PatchPositions PatchTree::get_deletion_patch_positions(const Triple& triple, int patch_id, bool override___, PatchPosition ___) const {
    return PatchPositions(
            deletion_count(Triple(triple.get_subject(), triple.get_predicate(), 0), patch_id).first,
            deletion_count(Triple(triple.get_subject(), 0, triple.get_object()), patch_id).first,
            deletion_count(Triple(triple.get_subject(), 0, 0), patch_id).first,
            deletion_count(Triple(0, triple.get_predicate(), triple.get_object()), patch_id).first,
            deletion_count(Triple(0, triple.get_predicate(), 0), patch_id).first,
            deletion_count(Triple(0, 0, triple.get_object()), patch_id).first,
            override___ ? ___ : deletion_count(Triple(0, 0, 0), patch_id).first
    );
}

PatchTreeTripleIterator* PatchTree::addition_iterator_from(long offset, int patch_id, const Triple& triple_pattern) const {
    kyotocabinet::DB::Cursor* cursor = tripleStore->getAdditionsTree(triple_pattern)->cursor();
    size_t size;
    const char* data = triple_pattern.serialize(&size);
    cursor->jump(data, size);
    delete[] data;
    PatchTreeIterator* it = new PatchTreeIterator(nullptr, cursor, get_spo_comparator());
    it->set_patch_filter(patch_id, true);
    it->set_triple_pattern_filter(triple_pattern);
    it->set_filter_local_changes(true);
    // TODO: If this this ridiculous loop becomes too inefficient, make an offset map
    PatchTreeKey key;
#ifdef COMPRESSED_ADD_VALUES
    PatchTreeAdditionValue value(max_patch_id);
#else
    PatchTreeAdditionValue value;
#endif
    PatchPosition count = tripleStore->get_addition_count(patch_id, triple_pattern);
    if (count && count <= offset) {
        // Invalidate the iterator if our offset was larger than the total count.
        it->getAdditionCursor()->jump_back();
    }
    while(offset-- > 0 && it->next_addition(&key, &value));
#ifdef COMPRESSED_ADD_VALUES
    return new PatchTreeTripleIterator(it, triple_pattern, max_patch_id);
#else
    return new PatchTreeTripleIterator(it, triple_pattern);
#endif
}

PatchTreeIterator* PatchTree::addition_iterator(const Triple &triple_pattern) const {
    kyotocabinet::DB::Cursor* cursor = tripleStore->getAdditionsTree(triple_pattern)->cursor();
    size_t size;
    const char* data = triple_pattern.serialize(&size);
    cursor->jump(data, size);
    delete[] data;
    PatchTreeIterator* it = new PatchTreeIterator(nullptr, cursor, get_spo_comparator());
    it->set_triple_pattern_filter(triple_pattern);
    return it;
}

PatchPosition PatchTree::addition_count(int patch_id, const Triple& triple_pattern) const {
    PatchPosition count = tripleStore->get_addition_count(patch_id, triple_pattern);
    if (!count) {
        // This means that the count was too low to be stored in the count db, so we count manually.
        PatchTreeTripleIterator* it;
        if (!patch_id) {
#ifdef COMPRESSED_ADD_VALUES
            it = new PatchTreeTripleIterator(addition_iterator(triple_pattern), triple_pattern, max_patch_id);
#else
            it = new PatchTreeTripleIterator(addition_iterator(triple_pattern), triple_pattern);
#endif
        } else {
            it = addition_iterator_from(0, patch_id, triple_pattern);
        }
        Triple* triple = new Triple();
        while (it->next(triple)) count++;
        delete triple;
        delete it;
        return count;
    }
    return count;
}

PatchTreeAdditionValue* PatchTree::get_addition_value(const Triple &triple) const {
    size_t ksp, vsp;
    const char* kbp = triple.serialize(&ksp);
    const char* vbp = tripleStore->getDefaultAdditionsTree()->get(kbp, ksp, &vsp);
    free((char*) kbp);
    if (vbp != nullptr) {
#ifdef COMPRESSED_ADD_VALUES
        PatchTreeAdditionValue* value = new PatchTreeAdditionValue(max_patch_id);
#else
        PatchTreeAdditionValue* value = new PatchTreeAdditionValue();
#endif
        value->deserialize(vbp, vsp);
        delete[] vbp;
        return value;
    }
    return nullptr;
}

PatchTreeKeyComparator* PatchTree::get_spo_comparator() const {
    return tripleStore->get_spo_comparator();
}

PatchElementComparator *PatchTree::get_element_comparator() const {
    return tripleStore->get_element_comparator();
}

int PatchTree::get_max_patch_id() const {
    return max_patch_id;
}

int PatchTree::get_min_patch_id() const {
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

// Explicit specialization is required
template PatchTreeDeletionValue* PatchTree::get_deletion_value_after(const Triple& triple_pattern) const;
template PatchTreeDeletionValueReduced* PatchTree::get_deletion_value_after(const Triple& triple_pattern) const;
template PatchTreeIteratorBase<PatchTreeDeletionValue>* PatchTree::iterator(const Triple* triple_pattern) const;
template PatchTreeIteratorBase<PatchTreeDeletionValueReduced>* PatchTree::iterator(const Triple* triple_pattern) const;
