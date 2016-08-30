#include "controller.h"
#include "snapshot_patch_iterator_triple_id.h"

Controller::Controller(int8_t kc_opts) : patchTreeManager(new PatchTreeManager(kc_opts)), snapshotManager(new SnapshotManager()) {}

Controller::~Controller() {
    delete patchTreeManager;
    delete snapshotManager;
}

TripleIterator* Controller::get(const Triple& triple_pattern, int offset, int patch_id) const {
    // Find the snapshot
    int snapshot_id = get_snapshot_manager()->get_latest_snapshot(patch_id);
    if(snapshot_id < 0) {
        //throw std::invalid_argument("No snapshot was found for version " + std::to_string(patch_id));
        return new EmptyTripleIterator();
    }
    HDT* snapshot = get_snapshot_manager()->get_snapshot(snapshot_id);

    // Simple case: We are requesting a snapshot, delegate lookup to that snapshot.
    IteratorTripleID* snapshot_it = SnapshotManager::search_with_offset(snapshot, triple_pattern, offset);
    if(snapshot_id == patch_id) {
        return new SnapshotTripleIterator(snapshot_it);
    }
    DictionaryManager *dict = get_snapshot_manager()->get_dictionary_manager(snapshot_id);

    // Otherwise, we have to prepare an iterator for a certain patch
    PatchTree* patchTree = get_patch_tree_manager()->get_patch_tree(patch_id, dict);
    if(patchTree == NULL) {
        return new SnapshotTripleIterator(snapshot_it);
    }
    PositionedTripleIterator* deletion_it;
    long added_offset = 0;
    bool check_offseted_deletions = true;
    std::pair<PatchPosition, Triple> deletion_count_data = patchTree->deletion_count(triple_pattern, patch_id);
    // This loop continuously determines new snapshot iterators until it finds one that contains
    // no new deletions with respect to the snapshot iterator from last iteration.
    // This loop is required to handle special cases like the one in the ControllerTest::EdgeCase1.
    // As worst-case, this loop will take O(n) (n:dataset size), as an optimization we can look
    // into storing long consecutive chains of deletions more efficiently.
    while(check_offseted_deletions) {
        if (snapshot_it->hasNext()) { // We have elements left in the snapshot we should apply deletions to
            // Determine the first triple in the original snapshot and use it as offset for the deletion iterator
            TripleID *tripleId = snapshot_it->next();
            Triple firstTriple(tripleId->getSubject(), tripleId->getPredicate(), tripleId->getObject());
            // TODO: statement below causes memory issue, can't delete a TripleID
            //delete tripleId;
            deletion_it = patchTree->deletion_iterator_from(firstTriple, patch_id, triple_pattern);

            // Calculate a new offset, taking into account deletions.
            PositionedTriple first_deletion_triple;
            long snapshot_offset = 0;
            if (deletion_it->next(&first_deletion_triple, true)) {
                snapshot_offset = first_deletion_triple.position;
            } else {
                if(deletion_count_data.first == 0) {
                    snapshot_offset = 0;
                } else {
                    bool is_smaller_than_first = patchTree->get_spo_comparator()->compare(firstTriple, deletion_count_data.second) < 0;
                    snapshot_offset = is_smaller_than_first ? 0 : deletion_count_data.first;
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
    IteratorTripleID* tmp_it = SnapshotManager::search_with_offset(snapshot, triple_pattern, 0);
    while (tmp_it->hasNext()) {
        tmp_it->next();
        snapshot_count++;
    }

    // TODO: as an optimization, we should construct this iterator in a lazy manner?
    long addition_offset = offset - snapshot_count + deletion_count_data.first;
    PatchTreeTripleIterator * addition_it = patchTree->addition_iterator_from(addition_offset, patch_id, triple_pattern);

    return new SnapshotPatchIteratorTripleID(snapshot_it, deletion_it, addition_it, patchTree->get_spo_comparator());
}

bool Controller::append(const Patch& patch, int patch_id, DictionaryManager* dict, ProgressListener* progressListener) {
    // TODO: this will require some changes when we implement automatic snapshot creation.
    return get_patch_tree_manager()->append(patch, patch_id, dict, progressListener);
}

PatchTreeManager* Controller::get_patch_tree_manager() const {
    return patchTreeManager;
}

SnapshotManager* Controller::get_snapshot_manager() const {
    return snapshotManager;
}

DictionaryManager *Controller::get_dictionary_manager(int patch_id) const {
    int snapshot_id = get_snapshot_manager()->get_latest_snapshot(patch_id);
    if(snapshot_id < 0) {
        throw std::invalid_argument("No snapshot has been created yet.");
    }
    return get_snapshot_manager()->get_dictionary_manager(snapshot_id);
}

void Controller::cleanup(Controller* controller) {
    // Delete patch files
    std::map<int, PatchTree*> patches = controller->get_patch_tree_manager()->get_patch_trees();
    std::map<int, PatchTree*>::iterator itP = patches.begin();
    while(itP != patches.end()) {
        int id = itP->first;
        std::remove(PATCHTREE_FILENAME(id, "spo_deletions").c_str());
        std::remove(PATCHTREE_FILENAME(id, "spo").c_str());
        std::remove(PATCHTREE_FILENAME(id, "pos").c_str());
        std::remove(PATCHTREE_FILENAME(id, "pso").c_str());
        std::remove(PATCHTREE_FILENAME(id, "sop").c_str());
        std::remove(PATCHTREE_FILENAME(id, "osp").c_str());
        itP++;
    }

    // Delete snapshot files
    std::map<int, HDT*> snapshots = controller->get_snapshot_manager()->get_snapshots();
    std::map<int, HDT*>::iterator itS = snapshots.begin();
    std::list<int> patchDictsToDelete;
    while(itS != snapshots.end()) {
        int id = itS->first;
        std::remove(SNAPSHOT_FILENAME_BASE(id).c_str());
        std::remove((SNAPSHOT_FILENAME_BASE(id) + ".index").c_str());

        patchDictsToDelete.push_back(id);
        itS++;
    }

    delete controller;

    // Delete dictionaries
    std::list<int>::iterator it;
    for(it=patchDictsToDelete.begin(); it!=patchDictsToDelete.end(); ++it) {
        DictionaryManager::cleanup(*it);
    }
}
