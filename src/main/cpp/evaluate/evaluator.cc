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
        size += filesize((SNAPSHOT_FILENAME_BASE(id) + ".index.v1.1"));
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

long long Evaluator::measure_lookup_version_materialized(Dictionary& dict, Triple triple_pattern, int offset, int patch_id, int limit, int replications, int& result_count) {
    long long total = 0;
    for (int i = 0; i < replications; i++) {
        int limit_l = limit;
        StopWatch st;
        TripleIterator* ti = controller->get_version_materialized(triple_pattern, offset, patch_id);
        // Dummy loop over iterator
        Triple t;
        while((limit_l == -2 || limit_l-- > 0) && ti->next(&t)) {
            t.get_subject(dict);
            t.get_predicate(dict);
            t.get_object(dict);
            result_count++;
        };
        delete ti;
        total += st.stopReal();
    }
    result_count /= replications;
    return total / replications;
}

long long Evaluator::measure_count_version_materialized(Triple triple_pattern, int patch_id, int replications) {
    long long total = 0;
    for (int i = 0; i < replications; i++) {
        StopWatch st;
        std::pair<size_t, ResultEstimationType> count = controller->get_version_materialized_count(triple_pattern, patch_id, true);
        total += st.stopReal();
    }
    return total / replications;
}

long long Evaluator::measure_lookup_delta_materialized(Dictionary& dict, Triple triple_pattern, int offset, int patch_id_start, int patch_id_end, int limit, int replications, int& result_count) {
    long long total = 0;
    for (int i = 0; i < replications; i++) {
        int limit_l = limit;
        StopWatch st;
        TripleDeltaIterator *ti = controller->get_delta_materialized(triple_pattern, offset, patch_id_start,
                                                                     patch_id_end);
        TripleDelta t;
        while ((limit_l == -2 || limit_l-- > 0) && ti->next(&t)) {
            t.get_triple()->get_subject(dict);
            t.get_triple()->get_predicate(dict);
            t.get_triple()->get_object(dict);
            result_count++;
        };
        delete ti;
        total += st.stopReal();
    }
    result_count /= replications;
    return total / replications;
}

long long Evaluator::measure_count_delta_materialized(Triple triple_pattern, int patch_id_start, int patch_id_end, int replications) {
    long long total = 0;
    for (int i = 0; i < replications; i++) {
        StopWatch st;
        std::pair<size_t, ResultEstimationType> count = controller->get_delta_materialized_count(triple_pattern, patch_id_start, patch_id_end, true);
        total += st.stopReal();
    }
    return total / replications;
}

long long Evaluator::measure_lookup_version(Dictionary& dict, Triple triple_pattern, int offset, int limit, int replications, int& result_count) {
    long long total = 0;
    for (int i = 0; i < replications; i++) {
        int limit_l = limit;
        StopWatch st;
        TripleVersionsIterator* ti = controller->get_version(triple_pattern, offset);
        TripleVersions t;
        while((limit_l == -2 || limit_l-- > 0) && ti->next(&t)) {
            t.get_triple()->get_subject(dict);
            t.get_triple()->get_predicate(dict);
            t.get_triple()->get_object(dict);
            result_count++;
        };
        delete ti;
        total += st.stopReal();
    }
    result_count /= replications;
    return total / replications;
}

long long Evaluator::measure_count_version(Triple triple_pattern, int replications) {
    long long total = 0;
    for (int i = 0; i < replications; i++) {
        StopWatch st;
        std::pair<size_t, ResultEstimationType> count = controller->get_version_count(triple_pattern, true);
        total += st.stopReal();
    }
    return total / replications;
}

void Evaluator::test_lookup(string s, string p, string o, int replications, int offset, int limit) {
    DictionaryManager *dict = controller->get_snapshot_manager()->get_dictionary_manager(0);
    Triple triple_pattern(s, p, o, dict);
    cout << "---PATTERN START: " << triple_pattern.to_string(*dict) << endl;

    cout << "--- ---VERSION MATERIALIZED" << endl;
    cout << "patch,offset,limit,count-ms,lookup-mus,results" << endl;
    for(int i = 0; i < patch_count; i++) {
        int result_count1 = 0;
        long dcount = measure_count_version_materialized(triple_pattern, i, replications);
        long d1 = measure_lookup_version_materialized(*dict, triple_pattern, offset, i, limit, replications, result_count1);
        cout << "" << i << "," << offset << "," << limit << "," << dcount << "," << d1 << "," << result_count1 << endl;
    }

    cout << "--- ---DELTA MATERIALIZED" << endl;
    cout << "patch_start,patch_end,offset,limit,count-ms,lookup-mus,results" << endl;
    for(int i = 0; i < patch_count; i++) {
        for(int j = 0; j < i; j+=i/2+1) {
            if (j > 0) j--;
            int result_count1 = 0;
            long dcount = measure_count_delta_materialized(triple_pattern, j, i, replications);
            long d1 = measure_lookup_delta_materialized(*dict, triple_pattern, offset, j, i, limit, replications, result_count1);
            cout << "" << j << "," << i << "," << offset << "," << limit << "," << dcount << "," << d1 << "," << result_count1 << endl;
        }
    }

    cout << "--- ---VERSION" << endl;
    cout << "offset,limit,count-ms,lookup-mus,results" << endl;
    int result_count1 = 0;
    long dcount = measure_count_version(triple_pattern, replications);
    long d1 = measure_lookup_version(*dict, triple_pattern, offset, limit, replications, result_count1);
    cout << "" << offset << "," << limit << "," << dcount << "," << d1 << "," << result_count1 << endl;
}

void Evaluator::cleanup_controller() {
    //Controller::cleanup(controller);
    delete controller;
}




void BearEvaluatorMS::init(string basePath, string patchesBasePatch, SnapshotCreationStrategy* strategy, int startIndex, int endIndex,
                           ProgressListener *progressListener) {
    controller = new Controller(basePath, strategy, TreeDB::TCOMPRESS);

    std::cout << patchesBasePatch << std::endl;

    cout << "---INSERTION START---" << endl;
    cout << "version,added,durationms,rate,accsize" << endl;
    DIR *dir;
    if ((dir = opendir(patchesBasePatch.c_str())) != NULL) {
        for (int i = startIndex; i <= endIndex; i++) {
            string versionname = to_string(i);
            NOTIFYMSG(progressListener, ("Version " + versionname + "\n").c_str());
            populate_controller_with_version(patch_count, patchesBasePatch, progressListener);
            patch_count++;
        }
        closedir(dir);
    }
    cout << "---INSERTION END---" << endl;
}

void BearEvaluatorMS::init_readonly(string basePath) {
    controller = new Controller(basePath, TreeDB::TCOMPRESS, true);
    patch_count = controller->get_number_versions();
}

void BearEvaluatorMS::test_lookup(string s, string p, string o, int replications, int offset, int limit) {
    TemporaryTriple triple_pattern(s, p, o);
    cout << "---PATTERN START: " << triple_pattern.to_string() << endl;

    cout << "--- ---VERSION MATERIALIZED" << endl;
    cout << "patch,offset,limit,count-ms,lookup-mus,results" << endl;
    for(int i = 0; i < patch_count; i++) {
        int result_count1 = 0;
        long dcount = measure_count_version_materialized(triple_pattern, i, replications);
        long d1 = measure_lookup_version_materialized(triple_pattern, offset, i, limit, replications, result_count1);
        cout << "" << i << "," << offset << "," << limit << "," << dcount << "," << d1 << "," << result_count1 << endl;
    }

//    cout << "--- ---DELTA MATERIALIZED" << endl;
//    cout << "patch_start,patch_end,offset,limit,count-ms,lookup-mus,results" << endl;
//    for(int i = 0; i < patch_count; i++) {
//        for(int j = 0; j < i; j+=i/2+1) {
//            if (j > 0) j--;
//            int result_count1 = 0;
//            long dcount = measure_count_delta_materialized(triple_pattern, j, i, replications);
//            long d1 = measure_lookup_delta_materialized(*dict, triple_pattern, offset, j, i, limit, replications, result_count1);
//            cout << "" << j << "," << i << "," << offset << "," << limit << "," << dcount << "," << d1 << "," << result_count1 << endl;
//        }
//    }

    cout << "--- ---VERSION" << endl;
    cout << "offset,limit,count-ms,lookup-mus,results" << endl;
    int result_count1 = 0;
    long dcount = measure_count_version(triple_pattern, replications);
    long d1 = measure_lookup_version(triple_pattern, offset, limit, replications, result_count1);
    cout << "" << offset << "," << limit << "," << dcount << "," << d1 << "," << result_count1 << endl;
}

void BearEvaluatorMS::cleanup_controller() {
    delete controller;
}

void BearEvaluatorMS::populate_controller_with_version(int patch_id, string path, ProgressListener *progressListener) {
    if (path.back() == '/') path.pop_back();

    std::string versions_ids(std::to_string(patch_id) + "-" + std::to_string(patch_id+1));
    std::string file_additions(path + k_path_separator + "alldata.CB.nt" + k_path_separator + "data-added_" + versions_ids + ".nt");
    std::string file_deletions(path + k_path_separator + "alldata.CB.nt" + k_path_separator + "data-deleted_" + versions_ids + ".nt");

    bool first = patch_id == 0;

    if (first) {
        file_additions = path + k_path_separator + "alldata.IC.nt" + k_path_separator + "1.nt";
        ifstream f(file_additions.c_str());
        if (!f.good()) {
            file_additions = path + k_path_separator + "alldata.IC.nt" + k_path_separator + "000001.nt";
            ifstream f2(file_additions.c_str());
            if (!f2.good()) {
                cerr << "Could not find the first file at location: " << file_additions << endl;
                return;
            }
        }
    }

    DictionaryManager* dict;

    CombinedTripleIterator *it_snapshot = nullptr;
    PatchElementIteratorCombined *it_patch = nullptr;
    if (first) {
        it_snapshot = new CombinedTripleIterator();
    } else {
        int snapshot_id = controller->get_snapshot_manager()->get_latest_snapshot(patch_id);
        controller->get_snapshot_manager()->load_snapshot(snapshot_id);
        dict = controller->get_snapshot_manager()->get_dictionary_manager(snapshot_id);
        it_patch = new PatchElementIteratorCombined(PatchTreeKeyComparator(comp_s, comp_p, comp_o, dict));
    }

    StopWatch st;
    NOTIFYMSG(progressListener, "Loading patch...\n");
    if (first) {
        it_snapshot->appendIterator(get_from_file(file_additions));
    } else {
        auto triples_it_add = get_from_file(file_additions);
        auto triples_it_del = get_from_file(file_deletions);
        auto patchIt_add = new PatchElementIteratorTripleStrings(dict, triples_it_add, true);
        auto patchIt_del = new PatchElementIteratorTripleStrings(dict, triples_it_del, false);
        it_patch->appendIterator(patchIt_add);
        it_patch->appendIterator(patchIt_del);
    }

    size_t added;
    if (first) {
        NOTIFYMSG(progressListener, "\nCreating snapshot...\n");
        std::cout.setstate(std::ios_base::failbit); // Disable cout info from HDT
        HDT *hdt = controller->get_snapshot_manager()->create_snapshot(patch_id, it_snapshot, BASEURI, progressListener);
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

std::ifstream::pos_type BearEvaluatorMS::patchstore_size(Controller *controller) {
    long size = 0;

    std::map<int, PatchTree*> patches = controller->get_patch_tree_manager()->get_patch_trees();
    auto itP = patches.begin();
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
    auto itS = snapshots.begin();
    while(itS != snapshots.end()) {
        controller->get_snapshot_manager()->get_dictionary_manager(itS->first)->save();
        int id = itS->first;
        size += filesize(SNAPSHOT_FILENAME_BASE(id));
        size += filesize((SNAPSHOT_FILENAME_BASE(id) + ".index.v1.1"));
        size += filesize((PATCHDICT_FILENAME_BASE(id)));
        itS++;
    }

    return size;
}

std::ifstream::pos_type BearEvaluatorMS::filesize(const string& file) {
    return std::ifstream(file.c_str(), std::ifstream::ate | std::ifstream::binary).tellg();
}

IteratorTripleString *BearEvaluatorMS::get_from_file(const string& file) {
    return new RDFParserNtriples(file.c_str(), NTRIPLES);;
}

long long
BearEvaluatorMS::measure_lookup_version_materialized(const TemporaryTriple& triple_pattern, int offset, int patch_id, int limit,
                                                     int replications, int &result_count) {

    int snapshot_id = controller->get_snapshot_manager()->get_latest_snapshot(patch_id);
    controller->get_snapshot_manager()->load_snapshot(snapshot_id);
    DictionaryManager* dict = controller->get_snapshot_manager()->get_dictionary_manager(snapshot_id);

    long long total = 0;
    for (int i = 0; i < replications; i++) {
        int limit_l = limit;
        StopWatch st;
        TripleIterator* ti = controller->get_version_materialized(triple_pattern, offset, patch_id);
        // Dummy loop over iterator
        Triple t;
        while((limit_l == -2 || limit_l-- > 0) && ti->next(&t)) {
            t.get_subject(*dict);
            t.get_predicate(*dict);
            t.get_object(*dict);
            result_count++;
        }
        delete ti;
        total += st.stopReal();
    }
    result_count /= replications;
    return total / replications;
}

long long
BearEvaluatorMS::measure_count_version_materialized(const TemporaryTriple& triple_pattern, int patch_id, int replications) {
    long long total = 0;
    for (int i = 0; i < replications; i++) {
        StopWatch st;
        std::pair<size_t, ResultEstimationType> count = controller->get_version_materialized_count(triple_pattern, patch_id, true);
        total += st.stopReal();
    }
    return total / replications;
}

//long long BearEvaluatorMS::measure_lookup_delta_materialized(const TemporaryTriple& triple_pattern, int offset, int patch_id_start,
//                                                         int patch_id_end, int limit, int replications,
//                                                         int &result_count) {
//    return 0;
//}
//
//long long
//BearEvaluatorMS::measure_count_delta_materialized(const TemporaryTriple& triple_pattern, int patch_id_start, int patch_id_end,
//                                              int replications) {
//    return 0;
//}

long long BearEvaluatorMS::measure_lookup_version(const TemporaryTriple& triple_pattern, int offset, int limit, int replications,
                                                  int &result_count) {
    long long total = 0;
    for (int i = 0; i < replications; i++) {
        int limit_l = limit;
        StopWatch st;
        TripleVersionsIteratorCombined* ti = controller->get_version(triple_pattern, offset);
        TripleVersionsString t;
        while((limit_l == -2 || limit_l-- > 0) && ti->next(&t)) {
            t.get_triple()->get_subject();
            t.get_triple()->get_predicate();
            t.get_triple()->get_object();
            result_count++;
        };
        delete ti;
        total += st.stopReal();
    }
    result_count /= replications;
    return total / replications;
}

long long BearEvaluatorMS::measure_count_version(const TemporaryTriple& triple_pattern, int replications) {
    long long total = 0;
    for (int i = 0; i < replications; i++) {
        StopWatch st;
        std::pair<size_t, ResultEstimationType> count = controller->get_version_count(triple_pattern, true);
        total += st.stopReal();
    }
    return total / replications;
}




