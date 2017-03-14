#include <iostream>
#include <dirent.h>
#include <util/StopWatch.hpp>
#include <rdf/RDFParserNtriples.hpp>

#include "../snapshot/combined_triple_iterator.h"

#include "evaluator.h"
#include "../simpleprogresslistener.h"

void Evaluator::init(string basePath, string patchesBasePatch, int startIndex, int endIndex, ProgressListener* progressListener) {
    controller = new Controller(basePath, TreeDB::TCOMPRESS);

    cout << "---INSERTION START---" << endl;
    cout << "version,added,durationms,rate,accsize" << endl;
    DIR *dir;
    if ((dir = opendir(patchesBasePatch.c_str())) != NULL) {
        for (int i = startIndex; i <= endIndex; i++) {
            string versionname = to_string(i);
            NOTIFYMSG(progressListener, ("Version " + versionname + "\n").c_str());
            string path = patchesBasePatch + k_path_separator + versionname;
            populate_controller_with_version(patch_count++, path, progressListener);
        }
        closedir(dir);
    }
    cout << "---INSERTION END---" << endl;
}

void Evaluator::populate_controller_with_version(int patch_id, string path, ProgressListener* progressListener) {
    std::smatch base_match;
    std::regex regex_additions("([a-z0-9]*).nt.additions.txt");
    std::regex regex_deletions("([a-z0-9]*).nt.deletions.txt");

    DictionaryManager *dict = controller->get_snapshot_manager()->get_dictionary_manager(0);
    bool first = patch_id == 0;
    CombinedTripleIterator* it_snapshot = new CombinedTripleIterator();
    PatchElementIteratorCombined* it_patch = new PatchElementIteratorCombined(PatchTreeKeyComparator(comp_s, comp_p, comp_o, dict));

    if (controller->get_max_patch_id() >= patch_id) {
        if (first) {
            NOTIFYMSG(progressListener, "Skipped constructing snapshot because it already exists, loaded instead.\n");
            controller->get_snapshot_manager()->load_snapshot(patch_id);
        } else {
            NOTIFYMSG(progressListener, "Skipped constructing patch because it already exists, loading instead...\n");
            DictionaryManager* dict_patch = controller->get_dictionary_manager(patch_id);
            if (controller->get_patch_tree_manager()->get_patch_tree(patch_id, dict_patch)->get_max_patch_id() < patch_id) {
                controller->get_patch_tree_manager()->load_patch_tree(patch_id, dict_patch);
            }
            NOTIFYMSG(progressListener, "Loaded!\n");
        }
        return;
    }

    DIR *dir;
    struct dirent *ent;
    StopWatch st;
    NOTIFYMSG(progressListener, "Loading patch...\n");
    if ((dir = opendir(path.c_str())) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            string filename = string(ent->d_name);
            string full_path = path + k_path_separator + filename;
            if (filename != "." && filename != "..") {
                bool additions = std::regex_match(filename, base_match, regex_additions);
                bool deletions = std::regex_match(filename, base_match, regex_deletions);
                NOTIFYMSG(progressListener, ("FILE: " + full_path + "\n").c_str());
                if (first && additions) {
                    it_snapshot->appendIterator(get_from_file(full_path));
                } else if(!first && (additions || deletions)) {
                    IteratorTripleString *subIt = get_from_file(full_path);
                    PatchElementIteratorTripleStrings* patchIt = new PatchElementIteratorTripleStrings(dict, subIt, additions);
                    it_patch->appendIterator(patchIt);
                }
            }
        }
        closedir(dir);
    }

    long long added;
    if (first) {
        NOTIFYMSG(progressListener, "\nCreating snapshot...\n");
        std::cout.setstate(std::ios_base::failbit); // Disable cout info from HDT
        HDT* hdt = controller->get_snapshot_manager()->create_snapshot(0, it_snapshot, BASEURI, progressListener);
        std::cout.clear();
        added = hdt->getTriples()->getNumberOfElements();

    } else {
        NOTIFYMSG(progressListener, "\nAppending patch...\n");
        controller->append(it_patch, patch_id, dict, false, progressListener);
        added = it_patch->getPassed();
    }
    long long duration = st.stopReal() / 1000;
    if (duration == 0) duration = 1; // Avoid division by 0
    long long rate = added / duration;
    std::ifstream::pos_type accsize = patchstore_size(controller);
    cout << patch_id << "," << added << "," << duration << "," << rate << "," << accsize << endl;

    delete it_snapshot;
    delete it_patch;
}

std::ifstream::pos_type Evaluator::patchstore_size(Controller* controller) {
    long size = 0;

    std::map<int, PatchTree*> patches = controller->get_patch_tree_manager()->get_patch_trees();
    std::map<int, PatchTree*>::iterator itP = patches.begin();
    while(itP != patches.end()) {
        int id = itP->first;
        size += filesize(PATCHTREE_FILENAME(id, "spo_deletions"));
        size += filesize(PATCHTREE_FILENAME(id, "pos_deletions"));
        size += filesize(PATCHTREE_FILENAME(id, "pso_deletions"));
        size += filesize(PATCHTREE_FILENAME(id, "sop_deletions"));
        size += filesize(PATCHTREE_FILENAME(id, "osp_deletions"));
        size += filesize(PATCHTREE_FILENAME(id, "spo_additions"));
        size += filesize(PATCHTREE_FILENAME(id, "pos_additions"));
        size += filesize(PATCHTREE_FILENAME(id, "pso_additions"));
        size += filesize(PATCHTREE_FILENAME(id, "sop_additions"));
        size += filesize(PATCHTREE_FILENAME(id, "osp_additions"));
        size += filesize(PATCHTREE_FILENAME(id, "count_additions"));
        itP++;
    }

    std::map<int, HDT*> snapshots = controller->get_snapshot_manager()->get_snapshots();
    controller->get_snapshot_manager()->get_dictionary_manager(0)->save();
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

std::ifstream::pos_type Evaluator::filesize(string file) {
    return std::ifstream(file.c_str(), std::ifstream::ate | std::ifstream::binary).tellg();
}

IteratorTripleString* Evaluator::get_from_file(string file) {
    return new RDFParserNtriples(file.c_str(), NTRIPLES);
}

long long Evaluator::measure_lookup_version_materialized(Triple triple_pattern, int offset, int patch_id, int limit, int replications, int& result_count) {
    long long total = 0;
    for (int i = 0; i < replications; i++) {
        int limit_l = limit;
        StopWatch st;
        TripleIterator* ti = controller->get_version_materialized(triple_pattern, offset, patch_id);
        // Dummy loop over iterator
        Triple t;
        while((limit_l == -2 || limit_l-- > 0) && ti->next(&t)) { result_count++; };
        delete ti;
        total += st.stopReal();
    }
    result_count /= replications;
    return total / replications;
}

long long Evaluator::measure_lookup_delta_materialized(Triple triple_pattern, int offset, int patch_id_start, int patch_id_end, int limit, int replications, int& result_count) {
    long long total = 0;
    for (int i = 0; i < replications; i++) {
        int limit_l = limit;
        StopWatch st;
        TripleDeltaIterator *ti = controller->get_delta_materialized(triple_pattern, offset, patch_id_start,
                                                                     patch_id_end);
        TripleDelta t;
        while ((limit_l == -2 || limit_l-- > 0) && ti->next(&t)) { result_count++; };
        delete ti;
        total += st.stopReal();
    }
    result_count /= replications;
    return total / replications;
}

long long Evaluator::measure_lookup_version(Triple triple_pattern, int offset, int limit, int replications, int& result_count) {
    long long total = 0;
    for (int i = 0; i < replications; i++) {
        int limit_l = limit;
        StopWatch st;
        TripleVersionsIterator* ti = controller->get_version(triple_pattern, offset);
        TripleVersions t;
        while((limit_l == -2 || limit_l-- > 0) && ti->next(&t)) { result_count++; };
        delete ti;
        total += st.stopReal();
    }
    result_count /= replications;
    return total / replications;
}

void Evaluator::test_lookup(string s, string p, string o, int replications) {
    DictionaryManager *dict = controller->get_snapshot_manager()->get_dictionary_manager(0);
    Triple triple_pattern(s, p, o, dict);
    cout << "---PATTERN START: " << triple_pattern.to_string(*dict) << endl;

    cout << "--- ---VERSION MATERIALIZED" << endl;
    cout << "patch,offset,lookup-mus-1,lookup-mus-50,lookup-mus-100,lookup-mus-inf,results1,results50,results100,resultsinf" << endl;
    for(int i = 0; i < patch_count; i++) {
        for(int offset = 0; offset < 1000; offset+=500) {
            int result_count1 = 0;
            int result_count50 = 0;
            int result_count100 = 0;
            int result_countinf = 0;
            long d1 = measure_lookup_version_materialized(triple_pattern, offset, i, 1, replications, result_count1);
            long d50 = measure_lookup_version_materialized(triple_pattern, offset, i, 50, replications, result_count50);
            long d100 = measure_lookup_version_materialized(triple_pattern, offset, i, 100, replications, result_count100);
            long dinf = measure_lookup_version_materialized(triple_pattern, offset, i, -2, replications, result_countinf);
            cout << "" << i << "," << offset << "," << d1 << "," << d50 << "," << d100 << "," << dinf << "," << result_count1 << "," << result_count50 << "," << result_count100 << "," << result_countinf << endl;
        }
    }

    cout << "--- ---DELTA MATERIALIZED" << endl;
    cout << "patch_start,patch_end,offset,lookup-mus-1,lookup-mus-50,lookup-mus-100,lookup-mus-inf,results1,results50,results100,resultsinf" << endl;
    for(int i = 0; i < patch_count; i++) {
        for(int j = 0; j < i; j+=i/2+1) {
            if (j > 0) j--;
            for (int offset = 0; offset < 1000; offset += 500) {
                int result_count1 = 0;
                int result_count50 = 0;
                int result_count100 = 0;
                int result_countinf = 0;
                long d1 = measure_lookup_delta_materialized(triple_pattern, offset, j, i, 1, replications, result_count1);
                long d50 = measure_lookup_delta_materialized(triple_pattern, offset, j, i, 50, replications, result_count50);
                long d100 = measure_lookup_delta_materialized(triple_pattern, offset, j, i, 100, replications, result_count100);
                long dinf = measure_lookup_delta_materialized(triple_pattern, offset, j, i, -2, replications, result_countinf);
                cout << "" << j << "," << i << "," << offset << "," << d1 << "," << d50 << "," << d100 << "," << dinf << "," << result_count1 << "," << result_count50 << "," << result_count100 << "," << result_countinf << endl;
            }
        }
    }

    cout << "--- ---VERSION" << endl;
    cout << "offset,lookup-mus-1,lookup-mus-50,lookup-mus-100,lookup-mus-inf,results1,results50,results100,resultsinf" << endl;
    for(int offset = 0; offset < 1000; offset+=500) {
        int result_count1 = 0;
        int result_count50 = 0;
        int result_count100 = 0;
        int result_countinf = 0;
        long d1 = measure_lookup_version(triple_pattern, offset, 1, replications, result_count1);
        long d50 = measure_lookup_version(triple_pattern, offset, 50, replications, result_count50);
        long d100 = measure_lookup_version(triple_pattern, offset, 100, replications, result_count100);
        long dinf = measure_lookup_version(triple_pattern, offset, -2, replications, result_countinf);
        cout << "" << offset << "," << d1 << "," << d50 << "," << d100 << "," << dinf << "," << result_count1 << "," << result_count50 << "," << result_count100 << "," << result_countinf << endl;
    }
}

void Evaluator::cleanup_controller() {
    //Controller::cleanup(controller);
    delete controller;
}
