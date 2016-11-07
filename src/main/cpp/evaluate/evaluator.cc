#include <iostream>
#include <dirent.h>
#include <util/StopWatch.hpp>
#include <rdf/RDFParserNtriples.hpp>

#include "../snapshot/combined_triple_iterator.h"

#include "evaluator.h"
#include "../simpleprogresslistener.h"

void Evaluator::init(string basePath, string patchesBasePatch, int startIndex, int endIndex, ProgressListener* progressListener) {
    controller = new Controller(basePath, HashDB::TCOMPRESS);

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
    PatchUnsorted patch;
    CombinedTripleIterator* it = new CombinedTripleIterator();

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
                int count = 0;
                bool additions = std::regex_match(filename, base_match, regex_additions);
                bool deletions = std::regex_match(filename, base_match, regex_deletions);
                NOTIFYMSG(progressListener, ("FILE: " + full_path + "\n").c_str());
                if (first && additions) {
                    it->appendIterator(get_from_file(full_path));
                } else if(!first && (additions || deletions)) {
                    IteratorTripleString *subIt = get_from_file(full_path);
                    while (subIt->hasNext()) {
                        TripleString* tripleString = subIt->next();
                        patch.add(PatchElement(Triple(tripleString->getSubject(), tripleString->getPredicate(), tripleString->getObject(), dict), additions));
                        count++;
                        if (count % 10000 == 0) {
                            NOTIFYLVL(progressListener, "Triple loading", count);
                        }
                        /*if (count > 100000) {
                            break;
                        }*/
                    }
                }
            }
        }
        closedir(dir);
    }

    long long added;
    if (first) {
        NOTIFYMSG(progressListener, "\nCreating snapshot...\n");
        std::cout.setstate(std::ios_base::failbit); // Disable cout info from HDT
        HDT* hdt = controller->get_snapshot_manager()->create_snapshot(0, it, BASEURI, new SimpleProgressListener());
        std::cout.clear();
        added = hdt->getTriples()->getNumberOfElements();
        delete it;
    } else {
        added = patch.get_size();
        NOTIFYMSG(progressListener, "\nAppending patch...\n");
        controller->append(patch, patch_id, dict, false, progressListener);
    }
    long long duration = st.stopReal() / 1000;
    if (duration == 0) duration = 1; // Avoid division by 0
    long long rate = added / duration;
    std::ifstream::pos_type accsize = patchstore_size(controller);
    cout << patch_id << "," << added << "," << duration << "," << rate << "," << accsize << endl;
}

std::ifstream::pos_type Evaluator::patchstore_size(Controller* controller) {
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

std::ifstream::pos_type Evaluator::filesize(string file) {
    return std::ifstream(file.c_str(), std::ifstream::ate | std::ifstream::binary).tellg();
}

IteratorTripleString* Evaluator::get_from_file(string file) {
    return new RDFParserNtriples(file.c_str(), NTRIPLES);
}

long long Evaluator::measure_lookup_version_materialized(Triple triple_pattern, int offset, int patch_id, int limit, int replications) {
    long long total = 0;
    for (int i = 0; i < replications; i++) {
        StopWatch st;
        //StopWatch st2;
        TripleIterator* ti = controller->get_version_materialized(triple_pattern, offset, patch_id);
        //cout << "A: " << (st2.stopReal()) << endl;st2.reset(); // TODO
        // Dummy loop over iterator
        Triple t;
        long count = 0;
        while((limit == -2 || limit-- > 0) && ti->next(&t)) { count++; };
        //cout << "Results: " << count << endl; // TODO
        //cout << "b: " << (st2.stopReal()) << endl;st2.reset(); // TODO
        total += st.stopReal() / 1000;
    }
    return total / replications;
}

long long Evaluator::measure_lookup_delta_materialized(Triple triple_pattern, int offset, int patch_id_start, int patch_id_end, int limit, int replications) {
    long long total = 0;
    for (int i = 0; i < replications; i++) {
        StopWatch st;
        TripleDeltaIterator *ti = controller->get_delta_materialized(triple_pattern, offset, patch_id_start,
                                                                     patch_id_end);
        TripleDelta t;
        long count = 0;
        while ((limit == -2 || limit-- > 0) && ti->next(&t)) { count++; };
        total += st.stopReal() / 1000;
    }
    return total / replications;
}

long long Evaluator::measure_lookup_version(Triple triple_pattern, int offset, int limit, int replications) {
    long long total = 0;
    for (int i = 0; i < replications; i++) {
        StopWatch st;
        TripleVersionsIterator* ti = controller->get_version(triple_pattern, offset);
        TripleVersions t;
        long count = 0;
        while((limit == -2 || limit-- > 0) && ti->next(&t)) { count++; };
        total += st.stopReal() / 1000;
    }
    return total / replications;
}

void Evaluator::test_lookup(string s, string p, string o, int replications) {
    DictionaryManager *dict = controller->get_snapshot_manager()->get_dictionary_manager(0);
    Triple triple_pattern(s, p, o, dict);
    cout << "---PATTERN START: " << triple_pattern.to_string(*dict) << endl;

    cout << "--- ---VERSION MATERIALIZED" << endl;
    cout << "patch,offset,lookup-ms-1,lookup-ms-50,lookup-ms-100,lookup-ms-inf" << endl;
    for(int i = 0; i < patch_count; i++) {
        for(int offset = 0; offset < 1000; offset+=500) {
            long d1 = measure_lookup_version_materialized(triple_pattern, offset, i, 1, replications);
            long d50 = measure_lookup_version_materialized(triple_pattern, offset, i, 50, replications);
            long d100 = measure_lookup_version_materialized(triple_pattern, offset, i, 100, replications);
            long dinf = measure_lookup_version_materialized(triple_pattern, offset, i, -2, replications);
            cout << "" << i << "," << offset << "," << d1 << "," << d50 << "," << d100 << "," << dinf << endl;
        }
    }

    /*cout << "--- ---DELTA MATERIALIZED" << endl;
    cout << "patch_start,patch_end,offset,lookup-ms-1,lookup-ms-50,lookup-ms-100,lookup-ms-inf" << endl;
    for(int i = 0; i < patch_count; i++) {
        for(int j = 0; j < i; j+=i/2+1) {
            if (j > 0) j--;
            for (int offset = 0; offset < 1000; offset += 500) {
                long d1 = measure_lookup_delta_materialized(triple_pattern, offset, j, i, 1, replications);
                long d50 = measure_lookup_delta_materialized(triple_pattern, offset, j, i, 50, replications);
                long d100 = measure_lookup_delta_materialized(triple_pattern, offset, j, i, 100, replications);
                long dinf = measure_lookup_delta_materialized(triple_pattern, offset, j, i, -2, replications);
                cout << "" << j << "," << i << "," << offset << "," << d1 << "," << d50 << "," << d100 << "," << dinf << endl;
            }
        }
    }*/

    cout << "--- ---VERSION" << endl;
    cout << "offset,lookup-ms-1,lookup-ms-50,lookup-ms-100,lookup-ms-inf" << endl;
    for(int offset = 0; offset < 1000; offset+=500) {
        long d1 = measure_lookup_version(triple_pattern, offset, 1, replications);
        long d50 = measure_lookup_version(triple_pattern, offset, 50, replications);
        long d100 = measure_lookup_version(triple_pattern, offset, 100, replications);
        long dinf = measure_lookup_version(triple_pattern, offset, -2, replications);
        cout << "" << offset << "," << d1 << "," << d50 << "," << d100 << "," << dinf << endl;
    }
}

void Evaluator::cleanup_controller() {
    //Controller::cleanup(controller);
    delete controller; // TODO
}
