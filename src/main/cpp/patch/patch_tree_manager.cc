#include <dirent.h>
#include <iostream>
#include <memory>
#include "patch_tree_manager.h"

PatchTreeManager::PatchTreeManager(string basePath, int8_t kc_opts, bool readonly) : basePath(basePath), max_loaded_patches(4), kc_opts(kc_opts), readonly(readonly) {
    detect_patch_trees();
}

PatchTreeManager::~PatchTreeManager() = default;

bool PatchTreeManager::append(PatchElementIterator* patch_it, int patch_id, std::shared_ptr<DictionaryManager> dict, bool check_uniqueness, ProgressListener* progressListener) {
    int patchtree_id = get_patch_tree_id(patch_id);
    std::shared_ptr<PatchTree> patchtree;
    if(patchtree_id < 0) {
        patchtree = construct_next_patch_tree(patch_id, dict);
    } else {
        patchtree = get_patch_tree(patchtree_id, dict);
    }
    if (check_uniqueness) {
        return patchtree->append(patch_it, patch_id, progressListener);
    } else {
        patchtree->append_unsafe(patch_it, patch_id, progressListener);
        return true;
    }
}

bool PatchTreeManager::append(const PatchSorted &patch, int patch_id, std::shared_ptr<DictionaryManager> dict, bool check_uniqueness,
                              ProgressListener *progressListener) {
    PatchElementIteratorVector* it = new PatchElementIteratorVector(&patch.get_vector());
    bool ret = append(it, patch_id, dict, check_uniqueness, progressListener);
    delete it;
    return ret;
}

const std::map<int, std::shared_ptr<PatchTree>>& PatchTreeManager::detect_patch_trees() {
    std::regex r("patchtree_([0-9]*).kct_spo_deletions");
    std::smatch base_match;
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
                    loaded_patches[std::stoi(base)] = nullptr; // Don't load the actual file, we do this lazily
                }
            }
        }
        closedir(dir);
    }
    return loaded_patches;
}

const std::map<int, std::shared_ptr<PatchTree>>& PatchTreeManager::get_patch_trees() const {
    return this->loaded_patches;
}

std::shared_ptr<PatchTree> PatchTreeManager::load_patch_tree(int patch_id_start, std::shared_ptr<DictionaryManager> dict) {
    update_cache(patch_id_start);
    loaded_patches[patch_id_start] = std::make_shared<PatchTree>(basePath, patch_id_start, dict, kc_opts, readonly);
    return loaded_patches[patch_id_start];
}

std::shared_ptr<PatchTree> PatchTreeManager::get_patch_tree(int patch_id_start, std::shared_ptr<DictionaryManager> dict) {
    if(patch_id_start < 0) {
        return nullptr;
    }
    auto it = loaded_patches.find(patch_id_start);
    if(it == loaded_patches.end()) {
        if(it == loaded_patches.begin()) {
            return nullptr; // We have an empty map
        }
        it--;
    }
    std::shared_ptr<PatchTree> patchtree = it->second;
    if(patchtree == nullptr) {
        return load_patch_tree(it->first, dict);
    }
    update_cache(it->first);
    return it->second;
}

std::shared_ptr<PatchTree> PatchTreeManager::construct_next_patch_tree(int patch_id_start, std::shared_ptr<DictionaryManager> dict) {
    return load_patch_tree(patch_id_start, dict);
}

int PatchTreeManager::get_patch_tree_id(int patch_id) const {
    // lower_bound does binary search in the map, so this is quite efficient.
    auto it = loaded_patches.lower_bound(patch_id);
    if(it == loaded_patches.end() || it->first > patch_id) {
        if(it == loaded_patches.begin()) {
            // In this case our patches did not start from id 0, but we still have to catch it.
            return -1;
        }
        it--;
    }
    return it->first;
}

Patch* PatchTreeManager::get_patch(int patch_id, std::shared_ptr<DictionaryManager> dict) {
    int patchtree_id = get_patch_tree_id(patch_id);
    if(patchtree_id < 0) {
        return new PatchSorted(dict);
    }
    return get_patch_tree(patchtree_id, dict)->reconstruct_patch(patch_id, true);
}

int PatchTreeManager::get_max_patch_id(std::shared_ptr<DictionaryManager> dict) {
    if (!loaded_patches.empty()) {
        std::map<int, std::shared_ptr<PatchTree>>::const_iterator it = loaded_patches.end();
        --it;
        std::shared_ptr<PatchTree> patchTree = it->second;
        if (patchTree == nullptr) {
            patchTree = load_patch_tree(it->first, dict);
        }
        return patchTree->get_max_patch_id();
    }
    return -1;
}

void PatchTreeManager::update_cache(int accessed_patch_id) {
    update_cache_internal(accessed_patch_id, max_loaded_patches);
}

void PatchTreeManager::update_cache_internal(int accessed_id, int iterations) {
    if (lru_map.find(accessed_id) == lru_map.end()) {
        if (lru_map.size() >= max_loaded_patches) {
            if (iterations > 0) {
                int lru_patchtree = lru_list.back();
                lru_list.pop_back();
                lru_map.erase(lru_patchtree);
                if (!loaded_patches[lru_patchtree].unique()) {
                    // the patchtree we want to unload is still used somewhere
                    // so we push it to the front of the list
                    lru_list.push_front(lru_patchtree);
                    lru_map[lru_patchtree] = lru_list.begin();
                    update_cache_internal(accessed_id, --iterations);
                    return;
                }
                loaded_patches[lru_patchtree] = nullptr;
            }
        }
    } else {
        lru_list.erase(lru_map[accessed_id]);
    }
    lru_list.push_front(accessed_id);
    lru_map[accessed_id] = lru_list.begin();
}


