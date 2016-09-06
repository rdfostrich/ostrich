#include <iostream>
#include <kchashdb.h>
#include <util/StopWatch.hpp>
#include <rdf/RDFParserNtriples.hpp>
#include <dirent.h>
#include <HDT.hpp>

#include "../../main/cpp/patch/patch_tree.h"
#include "../../main/cpp/snapshot/snapshot_manager.h"
#include "../../main/cpp/snapshot/vector_triple_iterator.h"
#include "../../main/cpp/controller/controller.h"
#include "../../main/cpp/snapshot/combined_triple_iterator.h"
#include "../../main/cpp/evaluate/evaluator.h"
#include "../../main/cpp/simpleprogresslistener.h"

using namespace std;
using namespace kyotocabinet;

Controller* setup_snapshot(int snapshot_size) {
    Controller* controller = new Controller(HashDB::TCOMPRESS);

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

    return controller;
}

void populate_controller(Controller* controller, int additions, int deletions, int addition_deletions, int& patch_id) {
    DictionaryManager *dict = controller->get_snapshot_manager()->get_dictionary_manager(0);
    if(additions > 0) {
        Patch patch1(dict);
        for (int i = 0; i < additions; i++) {
            string e = std::to_string(i * (patch_id + 1));
            //patch.add(PatchElement(Triple("a" + e, "a" + e, "a" + e), true));
            patch1.add(PatchElement(Triple("b" + e, "b" + e, "b" + e, dict), true));
            //patch.add(PatchElement(Triple("c" + e, "c" + e, "c" + e), true));
        }
        controller->append(patch1, patch_id++, dict);
    }

    if(deletions > 0) {
        Patch patch2(dict);
        for (int i = 0; i < deletions; i++) {
            string e = std::to_string(i * 10 * (patch_id + 1));
            patch2.add(PatchElement(Triple(e, e, e, dict), false));
        }
        controller->append(patch2, patch_id++, dict);
    }

    if(addition_deletions > 0) {
        Patch patch3(dict);
        for (int i = 0; i < addition_deletions; i++) {
            string e = std::to_string(i * (patch_id + 1));
            patch3.add(PatchElement(Triple("b" + e, "b" + e, "b" + e, dict), false));
        }
        controller->append(patch3, patch_id++, dict);
    }
}

long long test_insert_size(int snapshot_size, int additions, int deletions, int addition_deletions) {
    StopWatch st;
    int patch_id = 1;
    Controller* controller = setup_snapshot(snapshot_size);
    populate_controller(controller, additions, deletions, addition_deletions, patch_id);
    long long duration = st.stopReal() / 1000;
    Controller::cleanup(controller);
    return duration;
}

long long test_insert_count(Controller* controller, int additions, int deletions, int addition_deletions, int& count) {
    StopWatch st;
    populate_controller(controller, additions, deletions, addition_deletions, count);
    long long duration = st.stopReal() / 1000;
    return duration;
}

std::ifstream::pos_type filesize(string file) {
    return std::ifstream(file.c_str(), std::ifstream::ate | std::ifstream::binary).tellg();
}

std::ifstream::pos_type patchstore_size(Controller* controller) {
    long size = 0;

    std::map<int, PatchTree*> patches = controller->get_patch_tree_manager()->get_patch_trees();
    std::map<int, PatchTree*>::iterator itP = patches.begin();
    while(itP != patches.end()) {
        int id = itP->first;
        size += filesize(PATCHTREE_FILENAME(id, "spo_deletions"));
        size += filesize(PATCHTREE_FILENAME(id, "spo"));
        size += filesize(PATCHTREE_FILENAME(id, "pos"));
        size += filesize(PATCHTREE_FILENAME(id, "pso"));
        size += filesize(PATCHTREE_FILENAME(id, "sop"));
        size += filesize(PATCHTREE_FILENAME(id, "osp"));
        itP++;
    }

    std::map<int, HDT*> snapshots = controller->get_snapshot_manager()->get_snapshots();
    std::map<int, HDT*>::iterator itS = snapshots.begin();
    while(itS != snapshots.end()) {
        int id = itS->first;
        size += filesize(SNAPSHOT_FILENAME_BASE(id));
        size += filesize((SNAPSHOT_FILENAME_BASE(id) + ".index"));
        size += filesize((PATCHDICT_FILENAME_BASE(id)));
        itS++;
    }

    return size;
}

long test_lookup(Controller* controller, Triple triple_pattern, int offset, int patch_id, int limit) {
    StopWatch st;
    TripleIterator* ti = controller->get(triple_pattern, offset, patch_id);
    Triple t;
    while((limit == -2 || limit-- > 0) && ti->next(&t));
    return st.stopReal() / 1000;
}

int main() {
    // TODO: don't hardcode path to patches
    Evaluator evaluator;
    evaluator.init("/Users/rtaelman/nosync/patch-generator/dbpedia/patches", 2, 5, new SimpleProgressListener());

    evaluator.test_lookup("", "", "");
    evaluator.test_lookup("", "http://www.w3.org/2000/01/rdf-schema#label", "");
    evaluator.test_lookup("http://dbpedia.org/resource/Aldabrensis", "", "");

    evaluator.cleanup_controller();
}

int main_manual() {
    int duplicates = 1;

    test_insert_size(1000, 10, 0, 0); // WARM-UP

    // Insert (increasing patch-size)
    /*cout << "triples,additions-ms,deletions-ms,addition-deletions-ms" << endl;
    for(int i = 0; i < 1000; i += 10) {
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
    /*cout << "patches,additions-ms,deletions-ms,addition-deletions-ms" << endl;
    //for(int j = 0; j < duplicates; j++) {
    Controller* controller1 = setup_snapshot(1000);
    Controller* controller2 = setup_snapshot(1000);
    Controller* controller3 = setup_snapshot(1000);
    int i1 = 0;
    int i2 = 0;
    int i3 = 0;
    while (i1 < 1000) {
        long long total_additions = 0;
        long long total_deletions = 0;
        long long total_addition_deletions = 0;
        total_additions += test_insert_count(controller1, 100, 0, 0, i1);
        total_deletions += test_insert_count(controller2, 0, 100, 0, i2);
        total_addition_deletions += test_insert_count(controller3, 100, 0, 100, i3);
        cout << "" << i1 << "," << total_additions / duplicates << "," << total_deletions / duplicates<< "," << total_addition_deletions / duplicates << endl;
    }
    //}
    Controller::cleanup(controller1);
    Controller::cleanup(controller2);
    Controller::cleanup(controller3);*/

    // Lookup
    // Increasing patch id
    /*Controller* controller = setup_snapshot(10000);
    DictionaryManager *dict = controller->get_snapshot_manager()->get_dictionary_manager(0);
    int patches = 100;
    int i;
    while(i < patches) {
        cout << "" << i << " / " << patches << endl;
        populate_controller(controller, 0, 100, 0, i);
    }
    cout << "patch,lookup-ms-0-1,lookup-ms-0-50,lookup-ms-0-100,lookup-ms-100-1,lookup-ms-100-50,lookup-ms-100-100" << endl;
    for(int i = 0; i < patches; i ++) {
        long d0_1 = test_lookup(controller, Triple("", "", "", dict), 0, i, 1);
        long d0_50 = test_lookup(controller, Triple("", "", "", dict), 0, i, 50);
        long d0_100 = test_lookup(controller, Triple("", "", "", dict), 0, i, 100);

        long d100_1 = test_lookup(controller, Triple("", "", "", dict), 100, i, 1);
        long d100_50 = test_lookup(controller, Triple("", "", "", dict), 100, i, 50);
        long d100_100 = test_lookup(controller, Triple("", "", "", dict), 100, i, 100);
        cout << "" << i << ","
             << d0_1 << "," << d0_50 << "," << d0_100 << ","
             << d100_1 << "," << d100_50 << "," << d100_100
             << endl;
    }
    Controller::cleanup(controller);*/

    // File size
    /*Controller* controller = setup_snapshot(10000);
    int patches = 100;
    int i;
    cout << "patches,size" << endl;
    cout << "0," << patchstore_size(controller) << endl;
    while(i < patches) {
        // Force a flush to patchtree file
        delete controller;
        controller = new Controller(HashDB::TCOMPRESS);
        controller->get_snapshot_manager()->load_snapshot(0); // Force-reload snapshot 0 to make sure our dictmanager is loaded

        populate_controller(controller, 0, 200, 0, i);
        cout << "" << i << "," << patchstore_size(controller) << endl;
    }
    Controller::cleanup(controller);*/
}
