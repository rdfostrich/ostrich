#include <HDTManager.hpp>
#include <regex>
#include <dirent.h>
#include <hdt/BasicHDT.hpp>
#include "snapshot_manager.h"
#include "../../../../deps/hdt/hdt-lib/src/hdt/BasicModifiableHDT.hpp"
#include "../dictionary/dictionary_manager.h"

using namespace hdt;

SnapshotManager::SnapshotManager() : loaded_snapshots(detect_snapshots()), loaded_dictionaries(std::map<int, DictionaryManager*>()) {}

SnapshotManager::~SnapshotManager() {
    /*std::map<int, HDT*>::iterator it = loaded_snapshots.begin();
    while(it != loaded_snapshots.end()) {
        HDT* hdt = it->second;
        if(hdt != NULL) {
            delete hdt;
        }
        it++;
    }*/
    std::map<int, DictionaryManager*>::iterator it2 = loaded_dictionaries.begin();
    while(it2 != loaded_dictionaries.end()) {
        DictionaryManager* dict = it2->second;
        if(dict != NULL) {
            delete dict;
        }
        it2++;
    }
}

int SnapshotManager::get_latest_snapshot(int patch_id) {
    std::map<int, HDT*>::iterator it = loaded_snapshots.lower_bound(patch_id);
    if(it == loaded_snapshots.begin() && it == loaded_snapshots.end()) {
        return -1;
    }
    if(it == loaded_snapshots.end()) {
        it--;
    }
    while(it != loaded_snapshots.begin()) {
        if(it->first <= patch_id) {
            return it->first;
        }
        it--;
    };
    if(loaded_snapshots.begin()->first <= patch_id) {
        return loaded_snapshots.begin()->first;
    }
    return -1;
}

HDT* SnapshotManager::load_snapshot(int snapshot_id) {
    // TODO: We might want to look into unloading snapshots if they aren't used for a while. (using splay-tree/queue?)
    string fileName = SNAPSHOT_FILENAME_BASE(snapshot_id);
    loaded_snapshots[snapshot_id] = hdt::HDTManager::loadHDT(fileName.c_str());

    // load dictionary as well
    DictionaryManager* dict = new DictionaryManager(snapshot_id, loaded_snapshots[snapshot_id]->getDictionary());
    loaded_dictionaries[snapshot_id] = dict;

    return loaded_snapshots[snapshot_id];
}

HDT* SnapshotManager::get_snapshot(int snapshot_id) {
    if(snapshot_id < 0) {
        return NULL;
    }
    std::map<int, HDT*>::iterator it = loaded_snapshots.find(snapshot_id);
    if(it == loaded_snapshots.end()) {
        if(it == loaded_snapshots.begin()) {
            return NULL; // We have an empty map
        }
        it--;
    }
    HDT* snapshot = it->second;
    if(snapshot == NULL) {
        return load_snapshot(snapshot_id);
    }
    return it->second;
}

HDT* SnapshotManager::create_snapshot(int snapshot_id, IteratorTripleString* triples, string base_uri) {
    BasicHDT* basicHdt = new BasicHDT();
    basicHdt->loadFromTriples(triples, base_uri);
    basicHdt->saveToHDT(SNAPSHOT_FILENAME_BASE(snapshot_id).c_str());
    return load_snapshot(snapshot_id);
}

HDT* SnapshotManager::create_snapshot(int snapshot_id, string triples_file, string base_uri, RDFNotation notation) {
    BasicHDT* basicHdt = new BasicHDT();
    basicHdt->loadFromRDF(triples_file.c_str(), base_uri, notation);
    basicHdt->saveToHDT(SNAPSHOT_FILENAME_BASE(snapshot_id).c_str());
    return load_snapshot(snapshot_id);
}

std::map<int, HDT*> SnapshotManager::detect_snapshots() {
    std::regex r("snapshot_([0-9]*).hdt");
    std::smatch base_match;
    std::map<int, HDT*> snapshots = std::map<int, HDT*>();
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir(".")) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            if(std::regex_match(std::string(ent->d_name), base_match, r)) {
                // The first sub_match is the whole string; the next
                // sub_match is the first parenthesized expression.
                if (base_match.size() == 2) {
                    std::ssub_match base_sub_match = base_match[1];
                    std::string base = (std::string) base_sub_match.str();
                    snapshots[std::stoi(base)] = NULL; // Don't load the actual file, we do this lazily
                }
            }
        }
        closedir(dir);
    }
    return snapshots;
}

std::map<int, HDT*> SnapshotManager::get_snapshots() {
    return this->loaded_snapshots;
}


IteratorTripleID* SnapshotManager::search_with_offset(HDT *hdt, const Triple& triple_pattern, long offset) {
    TripleID tripleId(triple_pattern.get_subject(), triple_pattern.get_predicate(), triple_pattern.get_object());

    // The following step is needed to make sure that we are not using any triple component id's that are not available
    // in the HDT dict. Otherwise, HDT simply crashes.
    TripleString tripleString;
    hdt->getDictionary()->tripleIDtoTripleString(tripleId, tripleString);
    hdt->getDictionary()->tripleStringtoTripleID(tripleString, tripleId);

    IteratorTripleID* it = hdt->getTriples()->search(tripleId);
    if(it->canGoTo()) {
        try {
            it->goTo(offset);
            offset = 0;
        } catch (char const* error) {}
    }
    while(offset-- > 0 && it->hasNext()) it->next();
    return it;
}

DictionaryManager* SnapshotManager::get_dictionary_manager(int snapshot_id) {
    if(snapshot_id < 0) {
        return NULL;
    }
    std::map<int, DictionaryManager*>::iterator it = loaded_dictionaries.find(snapshot_id);
    if(it == loaded_dictionaries.end()) {
        if(it == loaded_dictionaries.begin()) {
            return NULL; // We have an empty map
        }
        it--;
    }
    return it->second;
}