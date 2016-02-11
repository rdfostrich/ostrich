#include <iostream>
#include <kchashdb.h>

#include "patch_tree.h"
#include "patch_elements.h"

using namespace std;
using namespace kyotocabinet;

PatchTree::PatchTree(string file_name) {
    // Set the triple comparator
    // TODO: this comparator should eventually be removed if serialization is fully implemented
    class ComparatorImpl : public Comparator {
        int32_t compare(const char* akbuf, size_t aksiz, const char* bkbuf, size_t bksiz) {
            PatchTreeKey* element1 = (PatchTreeKey*) akbuf;
            PatchTreeKey* element2 = (PatchTreeKey*) bkbuf;
            int comp_subject = element1->get_subject().compare(element2->get_subject());
            //cout << "comp: " << element1->subject << " ? " << element2->subject << " = " << comp_subject << endl; // TODO
            if(!comp_subject) {
                int comp_predicate = element1->get_predicate().compare(element2->get_predicate());
                if(!comp_predicate) {
                    return element1->get_object().compare(element2->get_object());
                }
                return comp_predicate;
            }
            return comp_subject;
        };
    };
    ComparatorImpl* comparator = new ComparatorImpl();
    db.tune_comparator(comparator);

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

int PatchTree::append(PatchElements patch, int patch_id) {
    for(int i = 0; i < patch.getSize(); i++) {
        PatchElement patchElement = patch.get(i);
        cout << "appending... " << patchElement.get_triple().get_subject() << endl; // TODO

        // Look up the value for the given triple key in the tree.
        size_t key_size, value_size;
        const char* raw_key = patchElement.get_triple().serialize(&key_size);
        PatchTreeValue value;
        const char* raw_value = db.get(raw_key, key_size, &value_size);
        value.deserialize(raw_value, value_size);

        // Modify the value
        int patch_position = 0; // TODO: the relative position in the list.
        if(!value.contains(patch_id)) {
            value.add(PatchTreeValueElement(patch_id, patch_position, patchElement.is_addition()));
        } else {
            cerr << "Already found a patch with id: " << patch_id << endl;
            return -1;
        }

        // Serialize the new value and store it
        size_t new_value_size;
        const char* new_raw_value = value.serialize(&new_value_size);
        db.set(raw_key, key_size, new_raw_value, new_value_size);
    }
    return 0;
}

PatchTreeIterator PatchTree::iterator(PatchTreeKey key) {
    DB::Cursor* cursor = db.cursor();
    cursor->jump((const char *) &key, sizeof(key));
    PatchTreeIterator patchTreeIterator(cursor);
    return patchTreeIterator;
}