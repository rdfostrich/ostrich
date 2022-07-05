#include <HDTManager.hpp>
#include <regex>
#include <dirent.h>
#include <hdt/BasicHDT.hpp>
#include "snapshot_manager.h"
#include "hdt/BasicModifiableHDT.hpp"
#include "../dictionary/dictionary_manager.h"

using namespace hdt;

SnapshotManager::SnapshotManager(string basePath, bool readonly, size_t cache_size) : basePath(basePath), max_loaded_snapshots(std::max((size_t)2,cache_size)), readonly(readonly) {
    detect_snapshots();
}

SnapshotManager::~SnapshotManager() = default;

int SnapshotManager::get_latest_snapshot(int patch_id) {
    auto it = loaded_snapshots.lower_bound(patch_id);
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

std::shared_ptr<HDT> SnapshotManager::load_snapshot(int snapshot_id) {
    // We check if a snapshot is already loaded for the given snapshot_id
    update_cache(snapshot_id);
    auto it = loaded_snapshots.find(snapshot_id);
    if (it != loaded_snapshots.end() && it->second) {
        return it->second;
    }

    string fileName = basePath + SNAPSHOT_FILENAME_BASE(snapshot_id);
    loaded_snapshots[snapshot_id] = std::shared_ptr<HDT>(hdt::HDTManager::mapIndexedHDT(fileName.c_str()));

    // load dictionary as well
    loaded_dictionaries[snapshot_id] = std::make_shared<DictionaryManager>(basePath, snapshot_id, loaded_snapshots[snapshot_id]->getDictionary(), readonly);

    return loaded_snapshots[snapshot_id];
}

std::shared_ptr<HDT> SnapshotManager::get_snapshot(int snapshot_id) {
    if(snapshot_id < 0) {
        return nullptr;
    }
    auto it = loaded_snapshots.find(snapshot_id);
    if(it == loaded_snapshots.end()) {
        if(it == loaded_snapshots.begin()) {
            return nullptr; // We have an empty map
        }
        it--;
    }
    if(it->second == nullptr) {
        return load_snapshot(snapshot_id);
    }
    update_cache(it->first);
    return it->second;
}

std::shared_ptr<HDT> SnapshotManager::create_snapshot(int snapshot_id, IteratorTripleString* triples, string base_uri, ProgressListener* listener) {
    BasicHDT* basicHdt = new BasicHDT();
    basicHdt->loadFromTriples(triples, base_uri, listener);
    basicHdt->saveToHDT((basePath + SNAPSHOT_FILENAME_BASE(snapshot_id)).c_str());
    delete basicHdt;
    return load_snapshot(snapshot_id);
}

std::shared_ptr<HDT> SnapshotManager::create_snapshot(int snapshot_id, string triples_file, string base_uri, RDFNotation notation) {
    BasicHDT* basicHdt = new BasicHDT();
    basicHdt->loadFromRDF(triples_file.c_str(), base_uri, notation);
    basicHdt->saveToHDT((basePath + SNAPSHOT_FILENAME_BASE(snapshot_id)).c_str());
    delete basicHdt;
    return load_snapshot(snapshot_id);
}

const std::map<int, std::shared_ptr<HDT>>& SnapshotManager::detect_snapshots() {
    std::regex r("snapshot_([0-9]*).hdt");
    std::smatch base_match;
    //std::map<int, HDT*> snapshots = std::map<int, HDT*>();
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir(basePath.c_str())) != nullptr) {
        while ((ent = readdir(dir)) != nullptr) {
            std::string dir_name = std::string(ent->d_name);
            if(std::regex_match(dir_name, base_match, r)) {
                // The first sub_match is the whole string; the next
                // sub_match is the first parenthesized expression.
                if (base_match.size() == 2) {
                    std::ssub_match base_sub_match = base_match[1];
                    std::string base = (std::string) base_sub_match.str();
                    int snapshot_id = std::stoi(base);
                    loaded_snapshots[snapshot_id] = nullptr; // Don't load the actual file, we do this lazily
                    loaded_dictionaries[snapshot_id] = nullptr; // We create a slot for the snapshot's dictionary
                }
            }
        }
        closedir(dir);
    }
    return loaded_snapshots;
}

std::vector<int> SnapshotManager::get_snapshots_ids() const {
    std::vector<int> ids;
    for (const auto& kv: loaded_snapshots) {
        ids.push_back(kv.first);
    }
    return ids;
}

IteratorTripleID* SnapshotManager::search_with_offset(std::shared_ptr<HDT> hdt, const Triple& triple_pattern, long offset) {
    TripleID tripleId(triple_pattern.get_subject(), triple_pattern.get_predicate(), triple_pattern.get_object());

    try {
        IteratorTripleID* it = hdt->getTriples()->search(tripleId);
        if(it->canGoTo()) {
            try {
                // If we goTo() with offset == 0, and HDT internal iterator is a MiddleWaveletIterator
                // the iterator position is adjusted to the wrong position (we should use goToStart() instead for this kind of iterator)
                // it is unclear if it is the intended behavior in HDT or if it is a bug with the MiddleWaveletIterator
                // So we don't goTo() if offset == 0, as it is not needed anyway
                if (offset != 0) {
                    it->goTo(offset);
                    offset = 0;
                }
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

std::shared_ptr<DictionaryManager> SnapshotManager::get_dictionary_manager(int snapshot_id) {
    if(snapshot_id < 0) {
        return nullptr;
    }
    auto it = loaded_dictionaries.find(snapshot_id);
    if(it == loaded_dictionaries.end()) {
        if(it == loaded_dictionaries.begin()) {
            return nullptr; // We have an empty map
        }
        it--;
    }
    if (it->second == nullptr || loaded_snapshots[snapshot_id] == nullptr) {
        // we make sure both the snapshot and dictionary are unloaded
        loaded_snapshots[snapshot_id] = nullptr;
        loaded_dictionaries[snapshot_id] = nullptr;
        // we load the snapshot
        load_snapshot(snapshot_id);
        return loaded_dictionaries[snapshot_id];
    }
    update_cache(snapshot_id);
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

void SnapshotManager::update_cache(int accessed_snapshot_id) {
    update_cache_internal(accessed_snapshot_id, lru_map.size());
}

void SnapshotManager::update_cache_internal(int accessed_id, int iterations) {
    if (lru_map.size() >= max_loaded_snapshots) {
        if (iterations >= 0) {
            int lru_snapshot_id = lru_list.back();
            lru_list.pop_back();
            lru_map.erase(lru_snapshot_id);
            if (!loaded_snapshots[lru_snapshot_id].unique() || !loaded_dictionaries[lru_snapshot_id].unique() || lru_snapshot_id == accessed_id) {
                // the snapshot or dictionary we want to unload is still used somewhere
                // so we push it to the front of the list
                lru_list.push_front(lru_snapshot_id);
                lru_map[lru_snapshot_id] = lru_list.begin();
                update_cache_internal(accessed_id, --iterations);
            } else {
                loaded_snapshots[lru_snapshot_id] = nullptr;
                loaded_dictionaries[lru_snapshot_id] = nullptr;
            }
        }
    }
    if (lru_map.find(accessed_id) != lru_map.end()) {
        lru_list.erase(lru_map[accessed_id]);
    }
    lru_list.push_front(accessed_id);
    lru_map[accessed_id] = lru_list.begin();
}

void SnapshotManager::set_cache_max_size(size_t new_size) {
    max_loaded_snapshots = std::max((size_t)2, new_size);
}

