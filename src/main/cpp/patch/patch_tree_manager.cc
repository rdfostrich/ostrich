#include <dirent.h>
#include <iostream>
#include "patch_tree_manager.h"

PatchTreeManager::PatchTreeManager(string basePath, int8_t kc_opts, bool readonly) : basePath(basePath), loaded_patches(detect_patch_trees()), kc_opts(kc_opts), readonly(readonly) {}

PatchTreeManager::~PatchTreeManager() {
    std::map<int, PatchTree*>::iterator it = loaded_patches.begin();
    while(it != loaded_patches.end()) {
        PatchTree* patchtree = it->second;
        if(patchtree != NULL) {
            delete patchtree;
        }
        it++;
    }
}

bool PatchTreeManager::append(PatchElementIterator* patch_it, int patch_id, DictionaryManager* dict, bool check_uniqueness, ProgressListener* progressListener) {
    int patchtree_id = get_patch_tree_id(patch_id);
    PatchTree* patchtree;
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

bool PatchTreeManager::append(const PatchSorted &patch, int patch_id, DictionaryManager *dict, bool check_uniqueness,
                              ProgressListener *progressListener) {
    PatchElementIteratorVector* it = new PatchElementIteratorVector(&patch.get_vector());
    bool ret = append(it, patch_id, dict, check_uniqueness, progressListener);
    delete it;
    return ret;
}

std::map<int, PatchTree*> PatchTreeManager::detect_patch_trees() const {
    std::regex r("patchtree_([0-9]*).kct_spo");
    std::smatch base_match;
    std::map<int, PatchTree*> trees = std::map<int, PatchTree*>();
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir(basePath.c_str())) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            if(std::regex_match(std::string(ent->d_name), base_match, r)) {
                // The first sub_match is the whole string; the next
                // sub_match is the first parenthesized expression.
                if (base_match.size() == 2) {
                    std::ssub_match base_sub_match = base_match[1];
                    std::string base = (std::string) base_sub_match.str();
                    trees[std::stoi(base)] = NULL; // Don't load the actual file, we do this lazily
                }
            }
        }
        closedir(dir);
    }
    return trees;
}

const std::map<int, PatchTree*>& PatchTreeManager::get_patch_trees() const {
    return this->loaded_patches;
}

PatchTree* PatchTreeManager::load_patch_tree(int patch_id_start, DictionaryManager* dict) {
    // TODO: We might want to look into unloading patch trees if they aren't used for a while. (using splay-tree/queue?)
    return loaded_patches[patch_id_start] = new PatchTree(basePath, patch_id_start, dict, kc_opts, readonly);
}

PatchTree* PatchTreeManager::get_patch_tree(int patch_id_start, DictionaryManager* dict) {
    if(patch_id_start < 0) {
        return NULL;
    }
    std::map<int, PatchTree*>::iterator it = loaded_patches.find(patch_id_start);
    if(it == loaded_patches.end()) {
        if(it == loaded_patches.begin()) {
            return NULL; // We have an empty map
        }
        it--;
    }
    PatchTree* patchtree = it->second;
    if(patchtree == NULL) {
        return load_patch_tree(patch_id_start, dict);
    }
    return it->second;
}

PatchTree* PatchTreeManager::construct_next_patch_tree(int patch_id_start, DictionaryManager* dict) {
    return load_patch_tree(patch_id_start, dict);
}

int PatchTreeManager::get_patch_tree_id(int patch_id) const {
    // lower_bound does binary search in the map, so this is quite efficient.
    std::map<int, PatchTree*>::const_iterator it = loaded_patches.lower_bound(patch_id);
    if(it == loaded_patches.end() || it->first > patch_id) {
        if(it == loaded_patches.begin()) {
            // In this case our patches did not start from id 0, but we still have to catch it.
            return -1;
        }
        it--;
    }
    return it->first;
}

Patch* PatchTreeManager::get_patch(int patch_id, DictionaryManager* dict) {
    int patchtree_id = get_patch_tree_id(patch_id);
    if(patchtree_id < 0) {
        return new PatchSorted(dict);
    }
    return get_patch_tree(patchtree_id, dict)->reconstruct_patch(patch_id, true);
}

int PatchTreeManager::get_max_patch_id(DictionaryManager* dict) {
    if (loaded_patches.size() > 0) {
        std::map<int, PatchTree*>::const_iterator it = loaded_patches.end();
        --it;
        PatchTree* patchTree = it->second;
        if (patchTree == NULL) {
            patchTree = load_patch_tree(it->first, dict);
        }
        return patchTree->get_max_patch_id();
    }
    return -1;
}
