#include <dirent.h>
#include <iostream>
#include "controller.h"
#include "snapshot_patch_iterator_triple_string.h"

Controller::Controller() : loaded_patches(detect_patch_trees()), snapshotManager(new SnapshotManager()) {}

Controller::~Controller() {
    std::map<int, PatchTree*>::iterator it = loaded_patches.begin();
    while(it != loaded_patches.end()) {
        PatchTree* patchtree = it->second;
        if(patchtree != NULL) {
            delete patchtree;
        }
        it++;
    }
    delete snapshotManager;
}

std::map<int, PatchTree*> Controller::detect_patch_trees() {
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

std::map<int, PatchTree*> Controller::get_patch_trees() {
    return this->loaded_patches;
}

PatchTree* Controller::load_patch_tree(int patch_id_start) {
    // TODO: We might want to look into unloading patch trees if they aren't used for a while. (using splay-tree/queue?)
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

TripleIterator* Controller::get(Triple triple_pattern, int offset, int patch_id) {
    // Find the snapshot
    int snapshot_id = snapshotManager->get_latest_snapshot(patch_id);
    if(snapshot_id < 0) {
        //throw std::invalid_argument("No snapshot was found for version " + std::to_string(patch_id));
        return new EmptyTripleIterator();
    }
    HDT* snapshot = snapshotManager->get_snapshot(snapshot_id);

    // Simple case: We are requesting a snapshot, delegate lookup to that snapshot.
    IteratorTripleString* snapshot_it = SnapshotManager::search_with_offset(snapshot, triple_pattern, offset);
    if(snapshot_id == patch_id) {
        return new SnapshotTripleIterator(snapshot_it);
    }

    // Otherwise, we have to prepare an iterator for a certain patch
    PatchTree* patchTree = get_patch_tree(patch_id);
    if(patchTree == NULL) {
        return new SnapshotTripleIterator(snapshot_it);
    }
    PositionedTripleIterator* deletion_it;
    long added_offset = 0;
    bool check_offseted_deletions = true;
    // This loop continuously determines new snapshot iterators until it finds one that contains
    // no new deletions with respect to the snapshot iterator from last iteration.
    // This loop is required to handle special cases like the one in the ControllerTest::EdgeCase1.
    // As worst-case, this loop will take O(n) (n:dataset size), as an optimization we can look
    // into storing long consecutive chains of deletions more efficiently.
    while(check_offseted_deletions) {
        if (snapshot_it->hasNext()) { // We have elements left in the snapshot we should apply deletions to
            // Determine the first triple in the original snapshot and use it as offset for the deletion iterator
            TripleString *tripleString = snapshot_it->next();
            Triple firstTriple(tripleString->getSubject(), tripleString->getPredicate(), tripleString->getObject());
            delete tripleString;
            deletion_it = patchTree->deletion_iterator_from(firstTriple, patch_id, triple_pattern);

            // Calculate a new offset, taking into account deletions.
            PositionedTriple first_deletion_triple;
            long snapshot_offset = 0;
            if (deletion_it->next(&first_deletion_triple, true)) {
                snapshot_offset = first_deletion_triple.position;
            } else {
                std::pair<PatchPosition, Triple> data = patchTree->deletion_count(triple_pattern, patch_id);
                if(data.first == 0) {
                    snapshot_offset = 0;
                } else {
                    bool is_smaller_than_first = firstTriple < data.second;
                    snapshot_offset = is_smaller_than_first ? 0 : data.first;
                }
            }
            long previous_added_offset = added_offset;
            added_offset = snapshot_offset;

            // Make a new snapshot iterator for the new offset
            delete snapshot_it;
            snapshot_it = SnapshotManager::search_with_offset(snapshot, triple_pattern, offset + added_offset);

            // Check if we need to loop again
            check_offseted_deletions = previous_added_offset < added_offset;
            if(check_offseted_deletions) {
                // TODO: it may be more efficient to just reuse the current deletion_it and offset in there.
                //       But probably not always.
                delete deletion_it;
            }
        } else {
            snapshot_it = NULL;
            deletion_it = NULL;
            check_offseted_deletions = false;
        }
    }

    // Calculate the offset for our addition iterator.
    // TODO: store this somewhere, we can't 'count' on the count provided by HDT, as this may not be exact.
    long snapshot_count = 0;
    IteratorTripleString* tmp_it = SnapshotManager::search_with_offset(snapshot, triple_pattern, 0);
    while (tmp_it->hasNext()) {
        tmp_it->next();
        snapshot_count++;
    }

    // TODO: as an optimization, we should construct this iterator in a lazy manner?
    long addition_offset = offset - snapshot_count + patchTree->deletion_count(triple_pattern, patch_id).first;
    PatchTreeTripleIterator * addition_it = patchTree->addition_iterator_from(addition_offset, patch_id, triple_pattern);

    return new SnapshotPatchIteratorTripleString(snapshot_it, deletion_it, addition_it);
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

SnapshotManager* Controller::get_snapshot_manager() {
    return snapshotManager;
}
