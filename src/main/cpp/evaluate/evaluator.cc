#include <iostream>
#include <dirent.h>
#include <util/StopWatch.hpp>
#include <rdf/RDFParserNtriples.hpp>

#include "../snapshot/combined_triple_iterator.h"

#include "evaluator.h"
#include "../simpleprogresslistener.h"
#include "../controller/statistics.h"

void Evaluator::init(string basePath, string patchesBasePatch, int startIndex, int endIndex, hdt::ProgressListener* progressListener) {
    controller = new Controller(basePath, kyotocabinet::TreeDB::TCOMPRESS);

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

void Evaluator::populate_controller_with_version(int patch_id, string path, hdt::ProgressListener* progressListener) {
    std::smatch base_match;
    std::regex regex_additions("([a-z0-9]*).nt.additions.txt");
    std::regex regex_deletions("([a-z0-9]*).nt.deletions.txt");

    std::shared_ptr<DictionaryManager> dict = controller->get_snapshot_manager()->get_dictionary_manager(0);
    bool first = patch_id == 0;
    CombinedTripleIterator* it_snapshot = new CombinedTripleIterator();
    PatchElementIteratorCombined* it_patch = new PatchElementIteratorCombined(PatchTreeKeyComparator(comp_s, comp_p, comp_o, dict));

    if (controller->get_max_patch_id() >= patch_id) {
        if (first) {
            NOTIFYMSG(progressListener, "Skipped constructing snapshot because it already exists, loaded instead.\n");
            controller->get_snapshot_manager()->load_snapshot(patch_id);
        } else {
            NOTIFYMSG(progressListener, "Skipped constructing patch because it already exists, loading instead...\n");
            std::shared_ptr<DictionaryManager> dict_patch = controller->get_dictionary_manager(patch_id);
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
                    hdt::IteratorTripleString *subIt = get_from_file(full_path);
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
        std::shared_ptr<hdt::HDT> hdt = controller->get_snapshot_manager()->create_snapshot(0, it_snapshot, BASEURI, progressListener);
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

    std::vector<int> patches = controller->get_patch_tree_manager()->get_patch_trees_ids();
    auto itP = patches.begin();
    while(itP != patches.end()) {
        int id = *itP;
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

    std::vector<int> snapshots = controller->get_snapshot_manager()->get_snapshots_ids();
    controller->get_snapshot_manager()->get_dictionary_manager(0)->save();
    auto itS = snapshots.begin();
    while(itS != snapshots.end()) {
        int id = *itS;
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

hdt::IteratorTripleString* Evaluator::get_from_file(string file) {
    return new hdt::RDFParserNtriples(file.c_str(), hdt::NTRIPLES);
}

long long Evaluator::measure_lookup_version_materialized(hdt::Dictionary& dict, Triple triple_pattern, int offset, int patch_id, int limit, int replications, int& result_count) {
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
        std::pair<size_t, hdt::ResultEstimationType> count = controller->get_version_materialized_count(triple_pattern, patch_id, true);
        total += st.stopReal();
    }
    return total / replications;
}

long long Evaluator::measure_lookup_delta_materialized(hdt::Dictionary& dict, Triple triple_pattern, int offset, int patch_id_start, int patch_id_end, int limit, int replications, int& result_count) {
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
        std::pair<size_t, hdt::ResultEstimationType> count = controller->get_delta_materialized_count(triple_pattern, patch_id_start, patch_id_end, true);
        total += st.stopReal();
    }
    return total / replications;
}

long long Evaluator::measure_lookup_version(hdt::Dictionary& dict, Triple triple_pattern, int offset, int limit, int replications, int& result_count) {
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
        std::pair<size_t, hdt::ResultEstimationType> count = controller->get_version_count(triple_pattern, true);
        total += st.stopReal();
    }
    return total / replications;
}

void Evaluator::test_lookup(string s, string p, string o, int replications, int offset, int limit) {
    std::shared_ptr<DictionaryManager> dict = controller->get_snapshot_manager()->get_dictionary_manager(0);
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
                           hdt::ProgressListener *progressListener) {
    controller = new Controller(basePath, strategy, kyotocabinet::TreeDB::TCOMPRESS);
//    cout << "---INSERTION START---" << endl;
//    cout << "version,added,durationms,rate,accsize" << endl;
    DIR *dir;
    if ((dir = opendir(patchesBasePatch.c_str())) != nullptr) {
        for (int i = startIndex; i <= endIndex; i++) {
            string versionname = to_string(i);
            NOTIFYMSG(progressListener, ("Version " + versionname + "\n").c_str());
            populate_controller_with_version(i-1, patchesBasePatch, progressListener);
//            patch_count++;
            patch_count = i-1;
        }
        closedir(dir);
    }
//    cout << "---INSERTION END---" << endl;
}

void BearEvaluatorMS::init_readonly(string basePath, bool warmup) {
    controller = new Controller(basePath, kyotocabinet::TreeDB::TCOMPRESS, true, 32);
    patch_count = controller->get_number_versions();

    if (warmup) {
        StringTriple pattern("", "", "");
        for (int i=0; i<patch_count; i++) {
            int snapshot_id = controller->get_snapshot_manager()->get_latest_snapshot(i);
            std::shared_ptr<DictionaryManager> dict = controller->get_snapshot_manager()->get_dictionary_manager(snapshot_id);
            TripleIterator* tmp_ti = controller->get_version_materialized(pattern, 0, i);
            Triple t;
            while (tmp_ti->next(&t)) {
                t.get_subject(*dict);
                t.get_predicate(*dict);
                t.get_object(*dict);
            }
            delete tmp_ti;
        }
    }
}

void BearEvaluatorMS::test_lookup(std::string s, std::string p, std::string o, int replications, int offset, int limit) {
    StringTriple triple_pattern(s, p, o);
    std::cout << "---PATTERN START: " << triple_pattern.to_string() << std::endl;

    std::cout << "--- ---VERSION MATERIALIZED" << std::endl;
    std::cout << "patch,offset,limit,count-ms,median-mus,lookup-mus,results" << std::endl;
    for(int i = 0; i < patch_count; i++) {
        int result_count1 = 0;
        uint64_t median_c;
        uint64_t dcount = measure_count_version_materialized(triple_pattern, i, replications, median_c);
        uint64_t median_t;
        uint64_t d1 = measure_lookup_version_materialized(triple_pattern, offset, i, limit, replications, result_count1, median_t);
        std::cout << "" << i << "," << offset << "," << limit << "," << dcount << "," << median_t << "," << d1 << "," << result_count1 << std::endl;
    }

    std:: cout << "--- ---DELTA MATERIALIZED" << endl;
    std::cout << "patch_start,patch_end,offset,limit,count-ms,median-mus,lookup-mus,results" << endl;
    for(int i = 1; i < patch_count; i++) {
        for(int j = 0; j <= 1; j++) {
            if (i > j) {
                int result_count1 = 0;
                uint64_t median_c;
                uint64_t dcount = measure_count_delta_materialized(triple_pattern, j, i, replications, median_c);
                uint64_t median_t;
                uint64_t d1 = measure_lookup_delta_materialized(triple_pattern, offset, j, i, limit, replications, result_count1, median_t);
                std::cout << "" << j << "," << i << "," << offset << "," << limit << "," << dcount << "," << median_t << "," << d1 << "," << result_count1 << std::endl;
            }
        }
    }

    std::cout << "--- ---VERSION" << endl;
    std::cout << "offset,limit,count-ms,median-mus,lookup-mus,results" << endl;
    int result_count1 = 0;
    uint64_t median_c;
    uint64_t dcount = measure_count_version(triple_pattern, replications, median_c);
    uint64_t median_t;
    uint64_t d1 = measure_lookup_version(triple_pattern, offset, limit, replications, result_count1, median_t);
    std::cout << "" << offset << "," << limit << "," << dcount << "," << median_t << "," << d1 << "," << result_count1 << std::endl;
}

void BearEvaluatorMS::compute_statistics() {
    Statistics stat(controller);
    std::cout << "Version,Change-ratio,Dynamicity,Growth-ratio,Entity-changes,Triple-to-entity-change,Object-updates" << std::endl;
    for (int i = 1; i < patch_count; i++) {
        std::set<StringTriple> consumed_triples;
        double cr = stat.change_ratio(i-1, i);
        double dyn = stat.dynamicity(i-1, i);
        double gr = stat.growth_ratio(i-1, i);
        size_t ec = stat.entity_changes(i-1, i);
        double tec = stat.triple_to_entity_change(i-1, i, &consumed_triples);
        size_t ou = stat.object_updates(i-1, i, &consumed_triples);
        std::cout << i << "," << cr << "," << dyn << "," << gr << "," << ec << "," << tec << "," << ou << std::endl;
    }
}

void BearEvaluatorMS::cleanup_controller() {
    delete controller;
}

void BearEvaluatorMS::populate_controller_with_version(int patch_id, std::string path, hdt::ProgressListener *progressListener) {
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

    std::shared_ptr<DictionaryManager> dict;

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
        std::shared_ptr<hdt::HDT> hdt = controller->get_snapshot_manager()->create_snapshot(patch_id, it_snapshot, BASEURI, progressListener);
        std::cout.clear();
        added = hdt->getTriples()->getNumberOfElements();
    } else {
        NOTIFYMSG(progressListener, "\nAppending patch...\n");
        std::cout.setstate(std::ios_base::failbit); // Disable cout info from HDT
        controller->append(it_patch, patch_id, dict, false, progressListener);
        std::cout.clear();
        added = it_patch->getPassed();
    }

    uint64_t duration = st.stopReal() / 1000;
    if (duration == 0) duration = 1; // Avoid division by 0
    uint64_t rate = added / duration;
    std::ifstream::pos_type accsize = patchstore_size(controller);
    cout << patch_id << "," << added << "," << duration << "," << rate << "," << accsize << endl;

    delete it_snapshot;
    delete it_patch;
}

std::ifstream::pos_type BearEvaluatorMS::patchstore_size(Controller *controller) {
    long size = 0;

    std::vector<int> patches = controller->get_patch_tree_manager()->get_patch_trees_ids();
    auto itP = patches.begin();
    while(itP != patches.end()) {
        int id = *itP;
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

    std::vector<int> snapshots = controller->get_snapshot_manager()->get_snapshots_ids();
    auto itS = snapshots.begin();
    while(itS != snapshots.end()) {
        controller->get_snapshot_manager()->get_dictionary_manager(*itS)->save();
        int id = *itS;
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

hdt::IteratorTripleString *BearEvaluatorMS::get_from_file(const string& file) {
    return new hdt::RDFParserNtriples(file.c_str(), hdt::NTRIPLES);
}

uint64_t
BearEvaluatorMS::measure_lookup_version_materialized(const StringTriple& triple_pattern, int offset, int patch_id, int limit,
                                                     int replications, int &result_count, uint64_t& median_t) {

    int snapshot_id = controller->get_snapshot_manager()->get_latest_snapshot(patch_id);
    controller->get_snapshot_manager()->load_snapshot(snapshot_id);
    std::shared_ptr<DictionaryManager> dict = controller->get_snapshot_manager()->get_dictionary_manager(snapshot_id);

    // Warmup
    Triple t;
    for (int i = 0; i < replications; i++) {
        TripleIterator *tmp_ti = controller->get_version_materialized(triple_pattern, offset, patch_id);
        while (tmp_ti->next(&t)) {
            t.get_subject(*dict);
            t.get_predicate(*dict);
            t.get_object(*dict);
        }
        delete tmp_ti;
    }

    // Query
    std::vector<uint64_t> times;
    uint64_t total = 0;
    for (int i = 0; i < replications; i++) {
        int limit_l = limit;
        StopWatch st;
        TripleIterator* ti = controller->get_version_materialized(triple_pattern, offset, patch_id);
        // Dummy loop over iterator
        while((limit_l == -2 || limit_l-- > 0) && ti->next(&t)) {
            t.get_subject(*dict);
            t.get_predicate(*dict);
            t.get_object(*dict);
            result_count++;
        }
        delete ti;
        uint64_t time = st.stopReal();
        times.push_back(time);
        total += time;
    }
    median_t = compute_median(times);
    result_count /= replications;
    return total / replications;
}

uint64_t
BearEvaluatorMS::measure_count_version_materialized(const StringTriple& triple_pattern, int patch_id, int replications, uint64_t& median_t) {
    // We ensure that the needed snapshot (and patchtree) are loaded prior to querying
    int snapshot_id = controller->get_snapshot_manager()->get_latest_snapshot(patch_id);
    controller->get_snapshot_manager()->load_snapshot(snapshot_id);
    if (snapshot_id != patch_id) {
        int patchtree_id = controller->get_patch_tree_manager()->get_patch_tree_id(patch_id);
        controller->get_patch_tree_manager()->load_patch_tree(patchtree_id, controller->get_dictionary_manager(patch_id));
    }

    std::vector<uint64_t> times;
    uint64_t total = 0;
    for (int i = 0; i < replications; i++) {
        StopWatch st;
        std::pair<size_t, hdt::ResultEstimationType> count = controller->get_version_materialized_count(triple_pattern, patch_id, true);
        uint64_t time = st.stopReal();
        times.push_back(time);
        total += time;
    }
    median_t = compute_median(times);
    return total / replications;
}

uint64_t BearEvaluatorMS::measure_lookup_version(const StringTriple& triple_pattern, int offset, int limit, int replications,
                                                  int &result_count, uint64_t& median_t) {

    // Warmup
    TripleVersions t;
    for (int i = 0; i < replications; i++) {
        TripleVersionsIterator* tmp_ti = controller->get_version(triple_pattern, offset);
        while(tmp_ti->next(&t)) {
            t.get_triple()->get_subject();
            t.get_triple()->get_predicate();
            t.get_triple()->get_object();
        }
        delete tmp_ti;
    }

    // Query
    std::vector<uint64_t> times;
    uint64_t total = 0;
    for (int i = 0; i < replications; i++) {
        int limit_l = limit;
        StopWatch st;
        TripleVersionsIterator* ti = controller->get_version(triple_pattern, offset);
        while((limit_l == -2 || limit_l-- > 0) && ti->next(&t)) {
            t.get_triple()->get_subject();
            t.get_triple()->get_predicate();
            t.get_triple()->get_object();
            result_count++;
        }
        delete ti;
        uint64_t time = st.stopReal();
        times.push_back(time);
        total += time;
    }
    result_count /= replications;
    return total / replications;
}

uint64_t BearEvaluatorMS::measure_count_version(const StringTriple& triple_pattern, int replications, uint64_t& median_t) {
    std::vector<uint64_t> times;
    uint64_t total = 0;
    for (int i = 0; i < replications; i++) {
        StopWatch st;
        std::pair<size_t, hdt::ResultEstimationType> count = controller->get_version_count(triple_pattern, true);
        uint64_t time = st.stopReal();
        times.push_back(time);
        total += time;
    }
    median_t = compute_median(times);
    return total / replications;
}

uint64_t
BearEvaluatorMS::measure_lookup_delta_materialized(const StringTriple &triple_pattern, int offset, int patch_id_start,
                                                   int patch_id_end, int limit, int replications, int &result_count, uint64_t& median_t) {

    TripleDelta t;
    // Warmup
    for (int i = 0; i < replications; i++) {
        TripleDeltaIterator *tmp_ti = controller->get_delta_materialized(triple_pattern, offset, patch_id_start,
                                                                         patch_id_end, false);
        while (tmp_ti->next(&t)) {
            t.get_triple()->get_subject(*(t.get_dictionary()));
            t.get_triple()->get_predicate(*(t.get_dictionary()));
            t.get_triple()->get_object(*(t.get_dictionary()));
        }
        delete tmp_ti;
    }

    // Query
    std::vector<uint64_t> times;
    uint64_t total = 0;
    for (int i = 0; i < replications; i++) {
        int limit_l = limit;
        StopWatch st;
        TripleDeltaIterator *ti = controller->get_delta_materialized(triple_pattern, offset, patch_id_start,
                                                                     patch_id_end, false);
        while ((limit_l == -2 || limit_l-- > 0) && ti->next(&t)) {
            t.get_triple()->get_subject(*(t.get_dictionary()));
            t.get_triple()->get_predicate(*(t.get_dictionary()));
            t.get_triple()->get_object(*(t.get_dictionary()));
            result_count++;
        }
        delete ti;
        uint64_t time = st.stopReal();
        times.push_back(time);
        total += time;
    }

    median_t = compute_median(times);
    result_count /= replications;
    return total / replications;
}

uint64_t BearEvaluatorMS::measure_count_delta_materialized(const StringTriple &triple_pattern, int patch_id_start,
                                                            int patch_id_end, int replications, uint64_t& median_t) {
    std::vector<uint64_t> times;
    uint64_t total = 0;
    for (int i = 0; i < replications; i++) {
        StopWatch st;
        std::pair<size_t, hdt::ResultEstimationType> count = controller->get_delta_materialized_count(triple_pattern, patch_id_start, patch_id_end, true);
        uint64_t time = st.stopReal();
        times.push_back(time);
        total += time;
    }
    median_t = compute_median(times);
    return total / replications;
}

uint64_t BearEvaluatorMS::compute_median(std::vector<uint64_t> values) {
    if (values.size() == 1) {
        return values[0];
    }
    const auto middle = values.begin() + values.size() / 2;
    std::nth_element(values.begin(), middle, values.end());
    if (values.size() % 2 == 0) {
        const auto left_middle = std::max_element(values.begin(), middle);
        return (*middle + *left_middle) / 2;
    }
    return *middle;
}
