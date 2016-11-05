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

void PatchTree::append_unsafe(const PatchIndexed& patch, int patch_id, ProgressListener* progressListener) {
    // Reconstruct the full patch and add the new elements.
    // We need this for finding their relative positions.
    NOTIFYMSG(progressListener, "Reconstructing...\n");
    PatchHashed* patch_existing = reconstruct_patch_hashed(patch_id);
    NOTIFYMSG(progressListener, "Adding internal patch...\n");
    PatchSorted* patch_combined = patch_existing->join_sorted(patch, get_element_comparator());
    delete patch_existing;

    // insert in other triplestore trees
    NOTIFYMSG(progressListener, ("Inserting " + to_string(patch_combined->get_size()) + " into auxiliary triple stores...\n").c_str());
    tripleStore->insertAddition(patch_combined, patch_id, progressListener);

    // Pre-calculate all inserting-patch triple positions.
    // We could instead do this during patch creation, but
    // I'm not convinced that this would speed up anything
    // since the total complexity will be the same.
    NOTIFYMSG(progressListener, "Precalculating patch positions...\n");
    unordered_map<Triple, long> inserting_patch_triple_positions;
    for (int i = 0; i < patch.get_size(); i++) {
        inserting_patch_triple_positions[patch.get(i).get_triple()] = i;
    }

    // Loop over all elements in this reconstructed patch
    // We don't only loop over the new elements, but all of them because
    // the already available elements might have a different relative patch position,
    // so these need to be updated.
    unordered_map<long, PatchPosition> sp_;
    unordered_map<long, PatchPosition> s_o;
    unordered_map<long, PatchPosition> s__;
    unordered_map<long, PatchPosition> _po;
    unordered_map<long, PatchPosition> _p_;
    unordered_map<long, PatchPosition> __o;
    PatchPosition ___ = 0;
    NOTIFYMSG(progressListener, ("Inserting " + to_string(patch_combined->get_size()) + " into main triple store...\n").c_str());
    for(int i = 0; i < patch_combined->get_size(); i++) {
        if (i % 10000 == 0) {
            NOTIFYLVL(progressListener, "Triple insertion", i);
        }
        PatchElement patchElement = patch_combined->get(i);
        unordered_map<Triple, long>::iterator it_in_inserting_patch = inserting_patch_triple_positions.find(patchElement.get_triple());
        long index_in_inserting_patch = it_in_inserting_patch != inserting_patch_triple_positions.end() ? it_in_inserting_patch->second : -1;
        bool is_in_inserting_patch = index_in_inserting_patch >= 0;
        bool is_addition = is_in_inserting_patch
                           ? patch.get(index_in_inserting_patch).is_addition()
                           : patchElement.is_addition();

        // If the triple is added by the inserting patch (`patch`), then we give priority to the type (+/-) from the inserting patch.
        if (!is_addition) {
            // Look up the value for the given triple key in the tree.
            size_t key_size, value_size;
            const char *raw_key = patchElement.get_triple().serialize(&key_size);
            PatchTreeDeletionValue value;
            const char *raw_value = tripleStore->getDeletionsTree()->get(raw_key, key_size, &value_size);
            if (raw_value) {
                value.deserialize(raw_value, value_size);
            }

            // Calculate the patch positions for all triple patterns (except for S P O, will be 0 anyways)
            PatchPositions patch_positions = patch_combined->positions(patchElement, sp_, s_o, s__, _po, _p_, __o, ___);
            // Add (or update) the value in the tree
            PatchTreeDeletionValueElement patchTreeValueElement(patch_id, patch_positions);
            if (patchElement.is_local_change()) {
                patchTreeValueElement.set_local_change();
            }
            value.add(patchTreeValueElement);

            // Serialize the new value and store it
            size_t new_value_size;
            const char *new_raw_value = value.serialize(&new_value_size);
            tripleStore->getDeletionsTree()->set(raw_key, key_size, new_raw_value, new_value_size);
        }
    }
    NOTIFYMSG(progressListener, "\nFinished patch insertion\n");

    if (patch_id > max_patch_id) {
        max_patch_id = patch_id;
    }
    delete patch_combined;
}

bool PatchTree::append(const PatchIndexed& patch, int patch_id, ProgressListener* progressListener) {
    PatchIterator* it = patch.iterator();
    while (it->has_next()) {
        const PatchElement& element = it->next();
        // We IGNORE the element type, because it makes no sense to have +/- for the same triple in the same patch!
        if(contains(element, patch_id, true)) {
            return false;
        }
    }
    delete it;
    append_unsafe(patch, patch_id, progressListener);
    return true;
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

std::pair<PatchPosition, Triple> PatchTree::deletion_count(const Triple& triple_pattern, int patch_id) const {
    DB::Cursor* cursor_deletions = tripleStore->getDeletionsTree()->cursor();

    // Try jumping backwards to the position where the triple_pattern matches
    // If this jump succeeds, we will be _before_ the last matching triple, so we keep the normal iteration order.
    // If this jump failed, we jump to the very last record, which will be _after_ the last matching triple, so we reverse the iteration order.
    size_t size;
    const char* data = triple_pattern.serialize(&size);
    bool hasJumped = cursor_deletions->jump_back(data, size);
    if (!hasJumped) {
        cursor_deletions->jump_back();
    }
    free((char*) data);

    PatchTreeIterator patchTreeIterator(cursor_deletions, NULL, get_spo_comparator());
    patchTreeIterator.set_patch_filter(patch_id, true);
    patchTreeIterator.set_triple_pattern_filter(triple_pattern);
    if (!hasJumped) patchTreeIterator.set_reverse(true);

    PatchTreeKey key;
    PatchTreeDeletionValue value;
    if(patchTreeIterator.next_deletion(&key, &value)) {
        return std::make_pair(value.get(patch_id).get_patch_positions().get_by_pattern(triple_pattern) + 1, key);
    }
    return std::make_pair((PatchPosition) 0, Triple());
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
