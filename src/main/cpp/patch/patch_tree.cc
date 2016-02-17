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

    // Loop over all elements in this reconstructed patch
    // We don't only loop over the new elements, but all of them because
    // the already available elements might have a different relative patch position,
    // so these need to be updated.
    for(int i = 0; i < existing_patch.get_size(); i++) {
        PatchElement patchElement = existing_patch.get(i);

        // Look up the value for the given triple key in the tree.
        size_t key_size, value_size;
        const char* raw_key = patchElement.get_triple().serialize(&key_size);
        PatchTreeValue value;
        const char* raw_value = db.get(raw_key, key_size, &value_size);
        if(raw_value) {
            value.deserialize(raw_value, value_size);
        }

        // Modify the patch positions for all triple patterns (except for S P O and ? ? ?, will be 0 anyways)
        PatchPositions patch_positions = existing_patch.positions(patchElement);
        // Add (or update) the value in the tree
        value.add(PatchTreeValueElement(patch_id, patch_positions, patchElement.is_addition()));

        // Serialize the new value and store it
        size_t new_value_size;
        const char* new_raw_value = value.serialize(&new_value_size);
        db.set(raw_key, key_size, new_raw_value, new_value_size);
    }
}

bool PatchTree::append(Patch patch, int patch_id) {
    for(long i = 0; i < patch.get_size(); i++) {
        // We IGNORE the element type, because it makes no sense to have +/- in the same patch!
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

Patch PatchTree::reconstruct_patch(int patch_id) {
    PatchTreeIterator it = iterator(patch_id, false);
    PatchTreeKey key;
    PatchTreeValue value;
    Patch patch;
    while(it.next(&key, &value)) {
        patch.add(PatchElement(key, value.is_addition(patch_id)));
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
    patchTreeIterator.set_reverse();

    PatchTreeKey key;
    PatchTreeValue value;
    while(patchTreeIterator.next(&key, &value)) {
        if((triple_pattern.get_subject() == "" || triple_pattern.get_subject() == key.get_subject())
           && (triple_pattern.get_predicate() == "" || triple_pattern.get_predicate() == key.get_predicate())
           && (triple_pattern.get_object() == "" || triple_pattern.get_object() == key.get_object())) {
            return value.get(patch_id).get_patch_positions().get_by_pattern(triple_pattern) + 1;
        }
    }
    return 0;
}
