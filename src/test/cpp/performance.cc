#include <iostream>
#include <kchashdb.h>
#include <util/StopWatch.hpp>

#include "../../main/cpp/patch/patch_tree.h"
#include "../../main/cpp/snapshot/snapshot_manager.h"
#include "../../main/cpp/snapshot/vector_triple_iterator.h"
#include "../../main/cpp/controller/controller.h"

#define BASEURI "<http://example.org>"

using namespace std;
using namespace kyotocabinet;

Controller* setup_controller(int snapshot_size, int additions, int deletions, int addition_deletions, int patch_count) {
    Controller* controller = new Controller();

    // Build a snapshot
    std::vector<TripleString> triples;
    for(int i = 0; i < snapshot_size; i++) {
        string e = std::to_string(i);
        triples.push_back(TripleString(e, e, e));
    }
    VectorTripleIterator *it = new VectorTripleIterator(triples);
    std::cout.setstate(std::ios_base::failbit); // Disable cout info from HDT
    controller->get_snapshot_manager()->create_snapshot(0, it, BASEURI);
    std::cout.clear();

    int patch_id = 0;
    for(int c = 0; c < patch_count; c++) {
        Patch patch1;
        for (int i = 0; i < additions; i++) {
            string e = std::to_string(i * (c + 1));
            //patch.add(PatchElement(Triple("a" + e, "a" + e, "a" + e), true));
            patch1.add(PatchElement(Triple("b" + e, "b" + e, "b" + e), true));
            //patch.add(PatchElement(Triple("c" + e, "c" + e, "c" + e), true));
        }
        controller->append(patch1, patch_id++);

        Patch patch2;
        for (int i = 0; i < deletions; i++) {
            string e = std::to_string(i * 10 * (c + 1));
            patch2.add(PatchElement(Triple(e, e, e), false));
        }
        controller->append(patch2, patch_id++);

        Patch patch3;
        for (int i = 0; i < addition_deletions; i++) {
            string e = std::to_string(i * (c + 1));
            patch3.add(PatchElement(Triple("b" + e, "b" + e, "b" + e), false));
        }
        controller->append(patch3, patch_id++);
    }

    return controller;
}

void cleanup_controller(Controller* controller) {
    // Delete patch files
    std::map<int, PatchTree*> patches = controller->get_patch_trees();
    std::map<int, PatchTree*>::iterator itP = patches.begin();
    while(itP != patches.end()) {
        int id = itP->first;
        std::remove(PATCHTREE_FILENAME(id, "spo").c_str());
        std::remove(PATCHTREE_FILENAME(id, "pos").c_str());
        std::remove(PATCHTREE_FILENAME(id, "pso").c_str());
        std::remove(PATCHTREE_FILENAME(id, "sop").c_str());
        std::remove(PATCHTREE_FILENAME(id, "osp").c_str());
        itP++;
    }

    // Delete snapshot files
    std::map<int, HDT*> snapshots = controller->get_snapshot_manager()->get_snapshots();
    std::map<int, HDT*>::iterator itS = snapshots.begin();
    while(itS != snapshots.end()) {
        int id = itS->first;
        std::remove(SNAPSHOT_FILENAME_BASE(id).c_str());
        std::remove((SNAPSHOT_FILENAME_BASE(id) + ".index").c_str());
        itS++;
    }

    delete controller;
}

long long test_insert_size(int snapshot_size, int additions, int deletions, int addition_deletions) {
    StopWatch st;
    Controller* controller = setup_controller(snapshot_size, additions, deletions, addition_deletions, 1);
    long long duration = st.stopReal() / 1000;
    cleanup_controller(controller);
    return duration;
}

long long test_insert_count(int snapshot_size, int additions, int deletions, int addition_deletions, int count) {
    StopWatch st;
    Controller* controller = setup_controller(snapshot_size, additions, deletions, addition_deletions, count);
    long long duration = st.stopReal() / 1000;
    cleanup_controller(controller);
    return duration;
}

long long test_lookup(Controller* controller, Triple triple_pattern, int offset, int patch_id, int limit) {
    StopWatch st;
    TripleIterator* ti = controller->get(triple_pattern, offset, patch_id);
    // Dummy loop over iterator
    Triple t;
    while(limit-- > 0 && ti->next(&t));

    return st.stopReal() / 1000;
}

int main() {
    int duplicates = 1;

    test_insert_size(1000, 10, 0, 0); // WARM-UP

    // Insert (increasing patch-size)
    cout << "triples,additions-ms,deletions-ms,addition-deletions-ms" << endl;
    /*for(int i = 0; i < 1000; i += 10) {
        long long total_additions = 0;
        long long total_deletions = 0;
        long long total_addition_deletions = 0;
        for(int j = 0; j < duplicates; j++) {
            total_additions += test_insert_size(1000, i, 0, 0);
            total_deletions += test_insert_size(1000, 0, i, 0);
            total_addition_deletions += test_insert_size(1000, i, 0, i);
        }
        cout << "" << i << "," << total_additions / duplicates << "," << total_deletions / duplicates<< "," << total_addition_deletions / duplicates << endl;
    }*/

    // Insert (increasing patch-count) (duration per patch)
    cout << "patches,additions-ms,deletions-ms,addition-deletions-ms" << endl;
    for(int i = 1; i < 1000; i += 10) {
        long long total_additions = 0;
        long long total_deletions = 0;
        long long total_addition_deletions = 0;
        for(int j = 0; j < duplicates; j++) {
            total_additions += test_insert_count(1000, 100, 0, 0, i) / i;
            total_deletions += test_insert_count(1000, 0, 100, 0, i) / i;
            total_addition_deletions += test_insert_count(1000, 100, 0, 100, i) / i;
        }
        cout << "" << i << "," << total_additions / duplicates << "," << total_deletions / duplicates<< "," << total_addition_deletions / duplicates << endl;
    }

    // Lookup
    // TODO
    // Increasing patch id
    /*Controller* controller = setup_controller(1000, 1000, 0, 0, 1);
    for(int i = 0; i < 1000; i += 10) {
        long d = test_lookup(controller, Triple("", "", ""), 0, i, 1);
        cout << "" << d << endl;
    }
    cleanup_controller(controller);*/
}
