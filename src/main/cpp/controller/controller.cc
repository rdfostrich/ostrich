#include "controller.h"
#include "snapshot_patch_iterator_triple_string.h"

Controller::Controller() : patchTreeManager(new PatchTreeManager()), snapshotManager(new SnapshotManager()) {}

Controller::~Controller() {
    delete patchTreeManager;
    delete snapshotManager;
}

TripleIterator* Controller::get(Triple triple_pattern, int offset, int patch_id) {
    // Find the snapshot
    int snapshot_id = get_snapshot_manager()->get_latest_snapshot(patch_id);
    if(snapshot_id < 0) {
        //throw std::invalid_argument("No snapshot was found for version " + std::to_string(patch_id));
        return new EmptyTripleIterator();
    }
    HDT* snapshot = get_snapshot_manager()->get_snapshot(snapshot_id);

    // Simple case: We are requesting a snapshot, delegate lookup to that snapshot.
    IteratorTripleString* snapshot_it = SnapshotManager::search_with_offset(snapshot, triple_pattern, offset);
    if(snapshot_id == patch_id) {
        return new SnapshotTripleIterator(snapshot_it);
    }

    // Otherwise, we have to prepare an iterator for a certain patch
    PatchTree* patchTree = get_patch_tree_manager()->get_patch_tree(patch_id);
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
    return get_patch_tree_manager()->append(patch, patch_id);
}

PatchTreeManager* Controller::get_patch_tree_manager() {
    return patchTreeManager;
}

SnapshotManager* Controller::get_snapshot_manager() {
    return snapshotManager;
}
