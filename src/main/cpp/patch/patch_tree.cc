#include <iostream>
#include <kchashdb.h>

#include "patch_tree.h"

using namespace std;
using namespace kyotocabinet;

PatchTree::PatchTree(string file_name) {
    // Open the database
    if (!db.open(file_name, HashDB::OWRITER | HashDB::OCREATE)) {
        cerr << "open error: " << db.error().name() << endl;
    }

    // Set the triple comparator
    class ComparatorImpl : public Comparator {
        int32_t compare(const char* akbuf, size_t aksiz, const char* bkbuf, size_t bksiz) {
            PatchTreeKey* element1 = (PatchTreeKey*) akbuf;
            PatchTreeKey* element2 = (PatchTreeKey*) bkbuf;
            int comp_subject = element1->subject.compare(element2->subject);
            if(!comp_subject) {
                int comp_predicate = element1->predicate.compare(element2->predicate);
                if(!comp_predicate) {
                    return element1->object.compare(element2->object);
                }
                return comp_predicate;
            }
            return comp_subject;
        };
    };
    ComparatorImpl comparator = ComparatorImpl();
    db.tune_comparator(&comparator);
};

PatchTree::~PatchTree() {
    // Close the database
    if (!db.close()) {
        cerr << "close error: " << db.error().name() << endl;
    }
}

void PatchTree::append(PatchElements* patch) {
    PatchElements* current = patch;
    while(current != NULL) {
        // TODO: write into tree
        cout << "appending... " << current->patchElement.triple.subject << endl;
        current = current->next;
    }
}