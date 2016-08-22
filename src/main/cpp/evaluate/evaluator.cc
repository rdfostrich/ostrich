#include <iostream>
#include <dirent.h>
#include <util/StopWatch.hpp>
#include <rdf/RDFParserNtriples.hpp>

#include "../snapshot/combined_triple_iterator.h"

#include "evaluator.h"
#include "../simpleprogresslistener.h"

void Evaluator::init(string patchesBasePatch, int startIndex, int endIndex, ProgressListener* progressListener) {
    controller = new Controller(HashDB::TCOMPRESS);

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
}

void Evaluator::populate_controller_with_version(int patch_id, string path, ProgressListener* progressListener) {
    std::smatch base_match;
    std::regex regex_additions("([a-z0-9]*).nt.additions.txt");
    std::regex regex_deletions("([a-z0-9]*).nt.deletions.txt");

    DictionaryManager *dict = controller->get_snapshot_manager()->get_dictionary_manager(0);
    bool first = patch_id == 0;
    Patch patch(dict);
    CombinedTripleIterator* it = new CombinedTripleIterator();

    if ((first && controller->get_snapshot_manager()->get_snapshot(patch_id) != NULL)
            || (!first && controller->get_patch_tree_manager()->get_patch_tree_id(patch_id) >= 0)) {
        cout << "Skipped loading patch because it already exists." << endl;
        return;
    }

    DIR *dir;
    struct dirent *ent;
    StopWatch st;
    cout << "Loading patch... " << endl;
    int count = 0;
    if ((dir = opendir(path.c_str())) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            string filename = string(ent->d_name);
            string full_path = path + k_path_separator + filename;
            if (filename != "." && filename != "..") {
                bool additions = std::regex_match(filename, base_match, regex_additions);
                bool deletions = std::regex_match(filename, base_match, regex_deletions);
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
                    }
                }
            }
        }
        closedir(dir);
    }
    long long duration = st.stopReal() / 1000;

    long long added;
    if (first) {
        //std::cout.setstate(std::ios_base::failbit); // Disable cout info from HDT
        HDT* hdt = controller->get_snapshot_manager()->create_snapshot(0, it, BASEURI);
        //std::cout.clear();
        added = hdt->getTriples()->getNumberOfElements();
        delete it;
    } else {
        added = patch.get_size();
        NOTIFYMSG(progressListener, "\nAppending patch...\n");
        controller->append(patch, patch_id, dict, progressListener);
    }
    cout << "  Added: " << added << endl;
    cout << "  Duration: " << duration << endl;
    long long rate = added / duration;
    cout << "  Rate: " << rate << " triples / ms" << endl;
}

IteratorTripleString* Evaluator::get_from_file(string file) {
    return new RDFParserNtriples(file.c_str(), NTRIPLES);
}

long long Evaluator::measure_lookup(Triple triple_pattern, int offset, int patch_id, int limit) {
    StopWatch st;
    TripleIterator* ti = controller->get(triple_pattern, offset, patch_id);
    // Dummy loop over iterator
    Triple t;
    while((limit == -2 || limit-- > 0) && ti->next(&t));
    return st.stopReal() / 1000;
}

void Evaluator::test_lookup(string s, string p, string o) {
    DictionaryManager *dict = controller->get_snapshot_manager()->get_dictionary_manager(0);
    Triple triple_pattern(s, p, o, dict);
    cout << ">> pattern: " << triple_pattern.to_string(*dict) << endl;
    cout << "patch,offset,lookup-ms-1,lookup-ms-50,lookup-ms-100,lookup-ms-inf" << endl;
    for(int i = 0; i < patch_count; i++) {
        for(int offset = 0; offset < 1000; offset+=100) {
            long d1 = measure_lookup(triple_pattern, offset, i, 1);
            long d50 = measure_lookup(triple_pattern, offset, i, 50);
            long d100 = measure_lookup(triple_pattern, offset, i, 100);
            long dinf = measure_lookup(triple_pattern, offset, i, -2);
            cout << "" << i << "," << offset << ","
            << d1 << "," << d50 << "," << d100 << "," << dinf << endl;
        }
    }
}

void Evaluator::cleanup_controller() {
    //Controller::cleanup(controller);
    delete controller; // TODO
}
