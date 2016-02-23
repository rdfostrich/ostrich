#include "controller.h"

Controller::Controller() {
    // TODO
}

Controller::~Controller() {
    std::map<int, PatchTree*>::iterator it = loaded_patches.begin();
    while(it != loaded_patches.end()) {
        PatchTree* patchtree = it->second;
        delete patchtree;
        it++;
    }
}

std::map<int, PatchTree*> Controller::detect_patch_trees() {
    // TODO
}

std::map<int, PatchTree*> Controller::get_patch_trees() {
    return this->loaded_patches;
}

PatchTree* Controller::load_patch_tree(int patch_id_start) {
    loaded_patches[patch_id_start] = new PatchTree(PATCHTREE_FILENAME(patch_id_start));
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

void Controller::construct_next_patch_tree(int patch_id_start) {
    load_patch_tree(patch_id_start);
}

int Controller::get_patch_tree_id(int patch_id) {
    // TODO
}

iterator<std::input_iterator_tag, Triple> Controller::get(Triple triple_pattern, int limit, int offset, int patch_id) {
    // TODO
}

bool Controller::append(Patch patch) {
    // TODO
}

void Controller::write_treemeta() {
    // TODO
}

void Controller::read_treemeta() {
    // TODO
}
