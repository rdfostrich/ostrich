#include <iostream>
#include <kchashdb.h>

#include "patch_tree.h"

using namespace std;
using namespace kyotocabinet;

PatchTree::PatchTree(string file_name) {
    // open the database
    if (!db.open(file_name, HashDB::OWRITER | HashDB::OCREATE)) {
        cerr << "open error: " << db.error().name() << endl;
    }
};

PatchTree::~PatchTree() {
    // close the database
    if (!db.close()) {
        cerr << "close error: " << db.error().name() << endl;
    }
}

void PatchTree::append(PatchElement* patch) {
    PatchElement* current = patch;
    while(current != NULL) {
        // TODO: write into tree
        cout << "appending... " << current->triple.subject << endl;
        current = current->next;
    }
}