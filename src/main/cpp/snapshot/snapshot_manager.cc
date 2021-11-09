#include <HDTManager.hpp>
#include <regex>
#include <dirent.h>
#include <hdt/BasicHDT.hpp>
#include "snapshot_manager.h"
#include "hdt/BasicModifiableHDT.hpp"
#include "../dictionary/dictionary_manager.h"

using namespace hdt;

SnapshotManager::SnapshotManager(string basePath, bool readonly) : basePath(basePath), max_loaded_snapshots(64), readonly(readonly), loaded_snapshots(detect_snapshots()), loaded_dictionaries(std::map<int, DictionaryManager*>()) {}

SnapshotManager::~SnapshotManager() {
    std::map<int, HDT*>::iterator it1 = loaded_snapshots.begin();
    while(it1 != loaded_snapshots.end()) {
        HDT* hdt = it1->second;
        if(hdt != NULL) {
            delete hdt;
        }
        it1++;
    }
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
    }
    if(loaded_snapshots.begin()->first <= patch_id) {
        return loaded_snapshots.begin()->first;
    }
    return -1;
}

HDT* SnapshotManager::load_snapshot(int snapshot_id) {
    // We check if a snapshot is already loaded for the given snapshot_id
    update_cache(snapshot_id);
    auto it = loaded_snapshots.find(snapshot_id);
    if (it != loaded_snapshots.end() && it->second) {
        return it->second;
    }

    string fileName = basePath + SNAPSHOT_FILENAME_BASE(snapshot_id);
    loaded_snapshots[snapshot_id] = hdt::HDTManager::mapIndexedHDT(fileName.c_str());

    // load dictionary as well
    DictionaryManager* dict = new DictionaryManager(basePath, snapshot_id, loaded_snapshots[snapshot_id]->getDictionary());
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
    update_cache(it->first);
    return it->second;
}

HDT* SnapshotManager::create_snapshot(int snapshot_id, IteratorTripleString* triples, string base_uri, ProgressListener* listener) {
    BasicHDT* basicHdt = new BasicHDT();
    basicHdt->loadFromTriples(triples, base_uri, listener);
    basicHdt->saveToHDT((basePath + SNAPSHOT_FILENAME_BASE(snapshot_id)).c_str());
    delete basicHdt;
    return load_snapshot(snapshot_id);
}

HDT* SnapshotManager::create_snapshot(int snapshot_id, string triples_file, string base_uri, RDFNotation notation) {
    BasicHDT* basicHdt = new BasicHDT();
    basicHdt->loadFromRDF(triples_file.c_str(), base_uri, notation);
    basicHdt->saveToHDT((basePath + SNAPSHOT_FILENAME_BASE(snapshot_id)).c_str());
    delete basicHdt;
    return load_snapshot(snapshot_id);
}

std::map<int, HDT*> SnapshotManager::detect_snapshots() {
    std::regex r("snapshot_([0-9]*).hdt");
    std::smatch base_match;
    std::map<int, HDT*> snapshots = std::map<int, HDT*>();
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir(basePath.c_str())) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            std::string dir_name = std::string(ent->d_name);
            if(std::regex_match(dir_name, base_match, r)) {
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

    try {
        IteratorTripleID* it = hdt->getTriples()->search(tripleId);
        if(it->canGoTo()) {
            try {
                it->goTo(offset);
                offset = 0;
            } catch (std::exception& e) {}
        }
        while(offset-- > 0 && it->hasNext()) it->next();
        return it;
    } catch (std::exception& e) {
        // HDT will crash when we are not using any triple component id's that are not available
        // in the HDT dict.
        // In that case, HDT will not contain any triples for the given pattern, so we return an empty iterator.
        return new IteratorTripleID();
    }
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

int SnapshotManager::get_max_snapshot_id() {
    if (loaded_snapshots.empty())
        return -1;
    auto it = loaded_snapshots.end();
    if(it == loaded_snapshots.begin() && it == loaded_snapshots.end()) {
        return it->first;
    }
    it--;
    return it->first;
}

void SnapshotManager::update_cache(int accessed_patch_id) {
    if (lru_map.find(accessed_patch_id) == lru_map.end()) {
        if (lru_map.size() == max_loaded_snapshots) {
            int id = lru_list.back();
            lru_list.pop_back();
            lru_map.erase(id);
            HDT* tmp = loaded_snapshots[id];
            DictionaryManager* tmp_d = loaded_dictionaries[id];
            loaded_snapshots[id] = nullptr;
            loaded_dictionaries[id] = nullptr;
            delete tmp;
            delete tmp_d;
        }
    } else {
        lru_list.erase(lru_map[accessed_patch_id]);
    }
    lru_list.push_front(accessed_patch_id);
    lru_map[accessed_patch_id] = lru_list.begin();
}
