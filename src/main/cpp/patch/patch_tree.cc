#include <iostream>
#include <kchashdb.h>

#include "patch_tree.h"
#include "patch_tree_key_comparator.h"

using namespace std;
using namespace kyotocabinet;

PatchTree::PatchTree(string file_name) {
    // Set the triple comparator
    keyComparator = new PatchTreeKeyComparator();
    db.tune_comparator(keyComparator);

    // Open the database
    if (!db.open(file_name, HashDB::OWRITER | HashDB::OCREATE)) {
        cerr << "open error: " << db.error().name() << endl;
    }
};

PatchTree::~PatchTree() {
    // Close the database
    if (!db.close()) {
        cerr << "close error: " << db.error().name() << endl;
    }
}

void PatchTree::append_unsafe(Patch patch, int patch_id) {
    // Reconstruct the full patch and add the new elements.
    // We need this for finding their relative positions.
    Patch existing_patch = reconstruct_patch(patch_id);
    existing_patch.addAll(patch);
    existing_patch = existing_patch.apply_local_changes();

    // Loop over all elements in this reconstructed patch
    // We don't only loop over the new elements, but all of them because
    // the already available elements might have a different relative patch position,
    // so these need to be updated.
    for(int i = 0; i < existing_patch.get_size(); i++) {
        PatchElement patchElement = existing_patch.get(i);
        long index_in_inserting_patch = patch.index_of_triple(patchElement.get_triple());
        bool is_in_inserting_patch = index_in_inserting_patch >= 0;

        // Look up the value for the given triple key in the tree.
        size_t key_size, value_size;
        const char* raw_key = patchElement.get_triple().serialize(&key_size);
        PatchTreeValue value;
        const char* raw_value = db.get(raw_key, key_size, &value_size);
        if(raw_value) {
            value.deserialize(raw_value, value_size);
        }

        // Calculate the patch positions for all triple patterns (except for S P O and ? ? ?, will be 0 anyways)
        PatchPositions patch_positions = existing_patch.positions(patchElement);
        // Add (or update) the value in the tree
        // If the triple is added by the inserting patch (`patch`), then we give priority to the type (+/-) from the inserting patch.
        PatchTreeValueElement patchTreeValueElement(patch_id, patch_positions, is_in_inserting_patch
                                                                               ? patch.get(index_in_inserting_patch).is_addition()
                                                                               : patchElement.is_addition());
        if(patchElement.is_local_change()) {
            patchTreeValueElement.set_local_change();
        }
        value.add(patchTreeValueElement);

        // Serialize the new value and store it
        size_t new_value_size;
        const char* new_raw_value = value.serialize(&new_value_size);
        db.set(raw_key, key_size, new_raw_value, new_value_size);
    }
}

bool PatchTree::append(Patch patch, int patch_id) {
    for(long i = 0; i < patch.get_size(); i++) {
        // We IGNORE the element type, because it makes no sense to have +/- for the same triple in the same patch!
        if(contains(patch.get(i), patch_id, true)) {
            return false;
        }
    }
    append_unsafe(patch, patch_id);
    return true;
}

bool PatchTree::contains(PatchElement patch_element, int patch_id, bool ignore_type) {
    PatchTreeKey key = patch_element.get_triple();
    size_t key_size, value_size;
    const char* raw_key = key.serialize(&key_size);
    const char* raw_value = db.get(raw_key, key_size, &value_size);

    // First, we check if the key is present
    bool ret = raw_value != NULL;
    if(ret) {
        // After that, we have to deserialize the value and check if for the given patch, the patch element
        // type (addition/deletion) is the same.
        PatchTreeValue value;
        value.deserialize(raw_value, value_size);
        long i = value.get_patchvalue_index(patch_id);
        ret = i >= 0;
        if(ret) {
            ret = ignore_type || value.get_patch(i).is_addition() == patch_element.is_addition();
        }
    }
    free((char*) raw_key);
    free((char*) raw_value);
    return ret;
}

Patch PatchTree::reconstruct_patch(int patch_id, bool ignore_local_changes) {
    PatchTreeIterator it = iterator(patch_id, false);
    it.set_filter_local_changes(ignore_local_changes);
    PatchTreeKey key;
    PatchTreeValue value;
    Patch patch;
    while(it.next(&key, &value)) {
        PatchElement patchElement(key, value.is_addition(patch_id));
        patchElement.set_local_change(value.is_local_change(patch_id));
        patch.add(patchElement);
    }
    return patch;
}

PatchTreeIterator PatchTree::iterator(PatchTreeKey* key) {
    DB::Cursor* cursor = db.cursor();
    size_t size;
    const char* data = key->serialize(&size);
    cursor->jump(data, size);
    free((char*) data);
    PatchTreeIterator patchTreeIterator(cursor);
    return patchTreeIterator;
}

PatchTreeIterator PatchTree::iterator(int patch_id, bool exact) {
    DB::Cursor* cursor = db.cursor();
    cursor->jump();
    PatchTreeIterator patchTreeIterator(cursor);
    patchTreeIterator.set_patch_filter(patch_id, exact);
    return patchTreeIterator;
}

PatchTreeIterator PatchTree::iterator(PatchTreeKey *key, int patch_id, bool exact) {
    DB::Cursor* cursor = db.cursor();
    size_t size;
    const char* data = key->serialize(&size);
    cursor->jump(data, size);
    free((char*) data);
    PatchTreeIterator patchTreeIterator(cursor);
    patchTreeIterator.set_patch_filter(patch_id, exact);
    return patchTreeIterator;
}

PatchPosition PatchTree::deletion_count(Triple triple_pattern, int patch_id) {
    DB::Cursor* cursor = db.cursor();
    cursor->jump_back();
    PatchTreeIterator patchTreeIterator(cursor);
    patchTreeIterator.set_patch_filter(patch_id, true);
    patchTreeIterator.set_type_filter(false);
    patchTreeIterator.set_triple_pattern_filter(triple_pattern);
    patchTreeIterator.set_reverse();

    PatchTreeKey key;
    PatchTreeValue value;
    while(patchTreeIterator.next(&key, &value)) {
        return value.get(patch_id).get_patch_positions().get_by_pattern(triple_pattern) + 1;
    }
    return 0;
}

PositionedTripleIterator PatchTree::deletion_iterator_from(Triple offset, int patch_id, Triple triple_pattern) {
    DB::Cursor* cursor = db.cursor();
    size_t size;
    const char* data = offset.serialize(&size);
    cursor->jump(data, size);
    free((char*) data);
    PatchTreeIterator* it = new PatchTreeIterator(cursor);
    it->set_patch_filter(patch_id, false);
    it->set_type_filter(false);
    it->set_triple_pattern_filter(triple_pattern);
    it->set_filter_local_changes(true);
    return PositionedTripleIterator(it, false, patch_id, triple_pattern);
}

PositionedTripleIterator PatchTree::addition_iterator_from(int offset, int patch_id, Triple triple_pattern) {
    DB::Cursor* cursor = db.cursor();
    cursor->jump();
    PatchTreeIterator* it = new PatchTreeIterator(cursor);
    it->set_patch_filter(patch_id, false);
    it->set_type_filter(true);
    it->set_triple_pattern_filter(triple_pattern);
    it->set_filter_local_changes(true);
    // TODO: this ridiculous loop won't be needed anymore when we add a more efficient addition indexing structure.
    PatchTreeKey key;
    PatchTreeValue value;
    while(offset-- > 0 && it->next(&key, &value));
    return PositionedTripleIterator(it, true, patch_id, triple_pattern);
}
