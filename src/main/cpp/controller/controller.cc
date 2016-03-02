#include <dirent.h>
#include <iostream>
#include "controller.h"

Controller::Controller() : loaded_patches(detect_patch_trees()) {}

Controller::~Controller() {
    std::map<int, PatchTree*>::iterator it = loaded_patches.begin();
    while(it != loaded_patches.end()) {
        PatchTree* patchtree = it->second;
        delete patchtree;
        it++;
    }
}

std::map<int, PatchTree*> Controller::detect_patch_trees() {
    std::regex r("patchtree_([0-9])*.kct_spo");
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

std::map<int, PatchTree*> Controller::get_patch_trees() {
    return this->loaded_patches;
}

PatchTree* Controller::load_patch_tree(int patch_id_start) {
    return loaded_patches[patch_id_start] = new PatchTree(PATCHTREE_FILENAME_BASE(patch_id_start));
}

PatchTree* Controller::get_patch_tree(int patch_id_start) {
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

PatchTree* Controller::construct_next_patch_tree(int patch_id_start) {
    return load_patch_tree(patch_id_start);
}

int Controller::get_patch_tree_id(int patch_id) {
    // lower_bound does binary search in the map, so this is quite efficient.
    std::map<int, PatchTree*>::iterator it = loaded_patches.lower_bound(patch_id);
    if(it == loaded_patches.end() || it->first > patch_id) {
        if(it == loaded_patches.begin()) {
            // In this case our patches did not start from id 0, but we still have to catch it.
            return -1;
        }
        it--;
    }
    return it->first;
}

Patch Controller::get_patch(int patch_id) {
    int patchtree_id = get_patch_tree_id(patch_id);
    if(patchtree_id < 0) {
        return Patch();
    }
    return get_patch_tree(patchtree_id)->reconstruct_patch(patch_id, true);
}

iterator<std::input_iterator_tag, Triple> Controller::get(Triple triple_pattern, int offset, int patch_id) {
    // TODO
}

bool Controller::append(Patch patch, int patch_id) {
    // TODO: this will require some changes when we implement automatic snapshot creation.
    int patchtree_id = get_patch_tree_id(patch_id);
    PatchTree* patchtree;
    if(patchtree_id < 0) {
        patchtree = construct_next_patch_tree(patch_id);
    } else {
        patchtree = get_patch_tree(patchtree_id);
    }
    return patchtree->append(patch, patch_id);
}
