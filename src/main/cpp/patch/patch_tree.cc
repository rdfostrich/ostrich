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

        // Modify the value
        int patch_position = existing_patch.position_of(patchElement);
        // Give an error for elements in `patch` that are already present in the tree.
        if(patch.position_of_strict(patchElement) == -1 // If this element is one of the patch elements we are simply updating (not one that we are newly adding now)
           || value.get_patchvalue_index(patch_id) == -1) { // If this element is part of the elements we are adding now AND the element is not present in the tree already
            value.add(PatchTreeValueElement(patch_id, patch_position, patchElement.is_addition()));
        } else {
            cerr << "Already found a patch element with id: " << patch_id << " Skipping this patch." << endl;
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