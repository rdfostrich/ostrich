#include <dirent.h>
#include <iostream>
#include "patch_tree_manager.h"
#include "../controller/snapshot_patch_iterator_triple_string.h"

PatchTreeManager::PatchTreeManager() : loaded_patches(detect_patch_trees()) {}

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

bool PatchTreeManager::append(const Patch& patch, int patch_id) {
    int patchtree_id = get_patch_tree_id(patch_id);
    PatchTree* patchtree;
    if(patchtree_id < 0) {
        patchtree = construct_next_patch_tree(patch_id);
    } else {
        patchtree = get_patch_tree(patchtree_id);
    }
    return patchtree->append(patch, patch_id);
}

std::map<int, PatchTree*> PatchTreeManager::detect_patch_trees() const {
    std::regex r("patchtree_([0-9]*).kct_spo");
    std::smatch base_match;
    std::map<int, PatchTree*> trees = std::map<int, PatchTree*>();
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

PatchTree* PatchTreeManager::load_patch_tree(int patch_id_start) {
    // TODO: We might want to look into unloading patch trees if they aren't used for a while. (using splay-tree/queue?)
    return loaded_patches[patch_id_start] = new PatchTree(PATCHTREE_FILENAME_BASE(patch_id_start));
}

PatchTree* PatchTreeManager::get_patch_tree(int patch_id_start) {
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
        return load_patch_tree(patch_id_start);
    }
    return it->second;
}

PatchTree* PatchTreeManager::construct_next_patch_tree(int patch_id_start) {
    return load_patch_tree(patch_id_start);
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

Patch PatchTreeManager::get_patch(int patch_id) {
    int patchtree_id = get_patch_tree_id(patch_id);
    if(patchtree_id < 0) {
        return Patch();
    }
    return get_patch_tree(patchtree_id)->reconstruct_patch(patch_id, true);
}
