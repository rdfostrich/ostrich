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

int PatchTree::append(Patch patch, int patch_id) {
    // TODO: reconstruct patch
    // TODO: merge 2 patches
    for(int i = 0; i < patch.get_size(); i++) {
        PatchElement patchElement = patch.get(i);

        // Look up the value for the given triple key in the tree.
        size_t key_size, value_size;
        const char* raw_key = patchElement.get_triple().serialize(&key_size);
        PatchTreeValue value;
        const char* raw_value = db.get(raw_key, key_size, &value_size);
        if(raw_value) {
            value.deserialize(raw_value, value_size);
        }

        // Modify the value
        int patch_position = 0; // TODO: the relative position in the merged patch.
        long existing_patch_index = value.get_patchvalue_index(patch_id);
        if(existing_patch_index == -1) {
            value.add(PatchTreeValueElement(patch_id, patch_position, patchElement.is_addition()));
        } else {
            cerr << "Already found a patch with id: " << patch_id << " Skipping this patch." << endl;
            return -1;
        }

        // Serialize the new value and store it
        size_t new_value_size;
        const char* new_raw_value = value.serialize(&new_value_size);
        db.set(raw_key, key_size, new_raw_value, new_value_size);
    }
    return 0;
}

Patch PatchTree::reconstruct_patch(int patch_id) {
    PatchTreeIterator it = iterator(patch_id);
    PatchTreeKey key;
    PatchTreeValue value;
    Patch patch;
    while(it.next(&key, &value)) {
        patch.add(PatchElement(key, value.get(patch_id).is_addition()));
    }
    return patch;
}

PatchTreeIterator PatchTree::iterator(PatchTreeKey* key) {
    DB::Cursor* cursor = db.cursor();
    cursor->jump((const char *) key, sizeof(key));
    PatchTreeIterator patchTreeIterator(cursor);
    return patchTreeIterator;
}

PatchTreeIterator PatchTree::iterator(int patch_id) {
    DB::Cursor* cursor = db.cursor();
    cursor->jump();
    PatchTreeIterator patchTreeIterator(cursor);
    patchTreeIterator.set_filter(patch_id);
    return patchTreeIterator;
}