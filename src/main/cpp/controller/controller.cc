#include <util/StopWatch.hpp>
#include "controller.h"
#include "snapshot_patch_iterator_triple_id.h"
#include "patch_builder_streaming.h"
#include <sys/stat.h>

Controller::Controller(string basePath, int8_t kc_opts, bool readonly) : patchTreeManager(new PatchTreeManager(basePath, kc_opts, readonly)), snapshotManager(new SnapshotManager(basePath, readonly)) {
    struct stat sb;
    if (!(stat(basePath.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode))) {
        throw std::invalid_argument("The provided path '" + basePath + "' is not a valid directory.");
    }
}

Controller::~Controller() {
    delete patchTreeManager;
    delete snapshotManager;
}

size_t Controller::get_version_materialized_count_estimated(const Triple& triple_pattern, int patch_id) const {
    return get_version_materialized_count(triple_pattern, patch_id, true).first;
}

std::pair<size_t, ResultEstimationType> Controller::get_version_materialized_count(const Triple& triple_pattern, int patch_id, bool allowEstimates) const {
    int snapshot_id = get_snapshot_manager()->get_latest_snapshot(patch_id);
    if(snapshot_id < 0) {
        return std::make_pair(0, EXACT);
    }

    HDT* snapshot = get_snapshot_manager()->get_snapshot(snapshot_id);
    IteratorTripleID* snapshot_it = SnapshotManager::search_with_offset(snapshot, triple_pattern, 0);
    size_t snapshot_count = snapshot_it->estimatedNumResults();
    if (!allowEstimates && snapshot_it->numResultEstimation() != EXACT) {
        snapshot_count = 0;
        while (snapshot_it->hasNext()) {
            snapshot_it->next();
            snapshot_count++;
        }
    }
    if(snapshot_id == patch_id) {
        return std::make_pair(snapshot_count, snapshot_it->numResultEstimation());
    }

    DictionaryManager *dict = get_snapshot_manager()->get_dictionary_manager(snapshot_id);
    PatchTree* patchTree = get_patch_tree_manager()->get_patch_tree(patch_id, dict);
    if(patchTree == NULL) {
        return std::make_pair(snapshot_count, snapshot_it->numResultEstimation());
    }

    std::pair<PatchPosition, Triple> deletion_count_data = patchTree->deletion_count(triple_pattern, patch_id);
    size_t addition_count = patchTree->addition_count(patch_id, triple_pattern);
    return std::make_pair(snapshot_count - deletion_count_data.first + addition_count, snapshot_it->numResultEstimation());
}

TripleIterator* Controller::get_version_materialized(const Triple &triple_pattern, int offset, int patch_id) const {
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
    PositionedTripleIterator* deletion_it = NULL;
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
            deletion_it = patchTree->deletion_iterator_from(firstTriple, patch_id, triple_pattern);

            // Calculate a new offset, taking into account deletions.
            PositionedTriple first_deletion_triple;
            long snapshot_offset = 0;
            if (deletion_it->next(&first_deletion_triple, true)) {
                snapshot_offset = first_deletion_triple.position;
            } else {
                snapshot_offset = deletion_count_data.first;
            }
            long previous_added_offset = added_offset;
            added_offset = snapshot_offset;

            // Make a new snapshot iterator for the new offset
            // TODO: look into reusing the snapshot iterator and applying a relative offset (NOTE: I tried it before, it's trickier than it seems...)
            delete snapshot_it;
            snapshot_it = SnapshotManager::search_with_offset(snapshot, triple_pattern, offset + added_offset);

            // Check if we need to loop again
            check_offseted_deletions = previous_added_offset < added_offset;
            if(check_offseted_deletions) {
                delete deletion_it;
                deletion_it = NULL;
            }
        } else {
            check_offseted_deletions = false;
        }
    }
    return new SnapshotPatchIteratorTripleID(snapshot_it, deletion_it, patchTree->get_spo_comparator(), snapshot, triple_pattern, patchTree, patch_id, offset, deletion_count_data.first);
}

std::pair<size_t, ResultEstimationType> Controller::get_delta_materialized_count(const Triple &triple_pattern, int patch_id_start, int patch_id_end, bool allowEstimates) const {
    return std::make_pair(get_delta_materialized(triple_pattern, 0, patch_id_start, patch_id_end)->get_count(), EXACT);
}

size_t Controller::get_delta_materialized_count_estimated(const Triple &triple_pattern, int patch_id_start, int patch_id_end) const {
    return get_delta_materialized_count(triple_pattern, patch_id_start, patch_id_end, true).second;
}

TripleDeltaIterator* Controller::get_delta_materialized(const Triple &triple_pattern, int offset, int patch_id_start,
                                                         int patch_id_end) const {
    if (patch_id_end <= patch_id_start) {
        return new EmptyTripleDeltaIterator();
    }

    // Find the snapshot
    int snapshot_id_start = get_snapshot_manager()->get_latest_snapshot(patch_id_start);
    int snapshot_id_end = get_snapshot_manager()->get_latest_snapshot(patch_id_end);
    if (snapshot_id_start < 0 || snapshot_id_end < 0) {
        return new EmptyTripleDeltaIterator();
    }

    // start = snapshot, end = snapshot
    if(snapshot_id_start == patch_id_start && snapshot_id_end == patch_id_end) {
        // TODO: implement this when multiple snapshots are supported
        throw std::invalid_argument("Multiple snapshots are not supported.");
    }

    // start = snapshot, end = patch
    if(snapshot_id_start == patch_id_start && snapshot_id_end != patch_id_end) {
        DictionaryManager *dict = get_snapshot_manager()->get_dictionary_manager(snapshot_id_end);
        if (snapshot_id_start == snapshot_id_end) {
            // Return iterator for the end patch relative to the start snapshot
            PatchTree* patchTree = get_patch_tree_manager()->get_patch_tree(patch_id_end, dict);
            if(patchTree == NULL) {
                throw std::invalid_argument("Could not find the given end patch id");
            }
            PatchTreeIterator* patchTreeIterator = patchTree->iterator(&triple_pattern);
            patchTreeIterator->set_patch_filter(patch_id_end, true);
            patchTreeIterator->set_filter_local_changes(true);
            patchTreeIterator->set_early_break(false);
            return (new ForwardPatchTripleDeltaIterator(patchTreeIterator))->offset(offset);
        } else {
            // TODO: implement this when multiple snapshots are supported
            throw std::invalid_argument("Multiple snapshots are not supported.");
        }
    }

    // start = patch, end = snapshot
    if(snapshot_id_start != patch_id_start && snapshot_id_end == patch_id_end) {
        // TODO: implement this when multiple snapshots are supported
        throw std::invalid_argument("Multiple snapshots are not supported.");
    }

    // start = patch, end = patch
    if(snapshot_id_start != patch_id_start && snapshot_id_end != patch_id_end) {
        DictionaryManager *dict = get_snapshot_manager()->get_dictionary_manager(snapshot_id_end);
        if (snapshot_id_start == snapshot_id_end) {
            // Return diff between two patches relative to the same snapshot
            PatchTree* patchTree = get_patch_tree_manager()->get_patch_tree(patch_id_end, dict);
            if(patchTree == NULL) {
                throw std::invalid_argument("Could not find the given end patch id");
            }
            PatchTreeIterator* patchTreeIterator = patchTree->iterator(&triple_pattern);
            patchTreeIterator->set_early_break(false);
            return (new FowardDiffPatchTripleDeltaIterator(patchTreeIterator, patch_id_start, patch_id_end))->offset(offset);
        } else {
            // TODO: implement this when multiple snapshots are supported
            throw std::invalid_argument("Multiple snapshots are not supported.");
        }
    }
    return nullptr;
}

std::pair<size_t, ResultEstimationType> Controller::get_version_count(const Triple &triple_pattern, bool allowEstimates) const {
    return std::make_pair(get_version(triple_pattern, 0)->get_count(), EXACT);
}

size_t Controller::get_version_count_estimated(const Triple &triple_pattern) const {
    return get_version_count(triple_pattern, true).second;
}

TripleVersionsIterator* Controller::get_version(const Triple &triple_pattern, int offset) const {
    // TODO: this will require some changes when we support multiple snapshots. (probably just a simple merge for all snapshots with what is already here)
    // Find the snapshot
    int snapshot_id = 0;
    HDT* snapshot = get_snapshot_manager()->get_snapshot(snapshot_id);
    IteratorTripleID* snapshot_it = SnapshotManager::search_with_offset(snapshot, triple_pattern, offset);
    DictionaryManager *dict = get_snapshot_manager()->get_dictionary_manager(snapshot_id);
    PatchTree* patchTree = get_patch_tree_manager()->get_patch_tree(snapshot_id, dict);
    if(patchTree == NULL) {
        throw std::invalid_argument("Could not find the given end patch id");
    }

    // Snapshots have already been offsetted, calculate the remaining offset.
    // After this, offset will only be >0 if we are past the snapshot elements and at the additions.
    if (snapshot_it->numResultEstimation() == EXACT) {
        offset -= snapshot_it->estimatedNumResults();
        if (offset <= 0) {
            offset = 0;
        } else {
            delete snapshot_it;
            snapshot_it = NULL;
        }
    } else {
        IteratorTripleID *tmp_it = SnapshotManager::search_with_offset(snapshot, triple_pattern, 0);
        while (tmp_it->hasNext() && offset > 0) {
            tmp_it->next();
            offset--;
        }
        delete tmp_it;
    }

    return (new TripleVersionsIterator(triple_pattern, snapshot_it, patchTree))->offset(offset);
}

bool Controller::append(PatchElementIterator* patch_it, int patch_id, DictionaryManager* dict, bool check_uniqueness, ProgressListener* progressListener) {
    // TODO: this will require some changes when we implement automatic snapshot creation.
    return get_patch_tree_manager()->append(patch_it, patch_id, dict, check_uniqueness, progressListener);
}

bool Controller::append(const PatchSorted& patch, int patch_id, DictionaryManager *dict, bool check_uniqueness,
                        ProgressListener *progressListener) {
    PatchElementIteratorVector* it = new PatchElementIteratorVector(&patch.get_vector());
    bool ret = append(it, patch_id, dict, check_uniqueness, progressListener);
    delete it;
    return ret;
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
    get_snapshot_manager()->load_snapshot(snapshot_id);
    return get_snapshot_manager()->get_dictionary_manager(snapshot_id);
}

int Controller::get_max_patch_id() {
    get_snapshot_manager()->get_snapshot(0); // Make sure our first snapshot is loaded, otherwise KC might get intro trouble while reorganising since it needs the dict for that.
    int max_patch_id = get_patch_tree_manager()->get_max_patch_id(get_snapshot_manager()->get_dictionary_manager(0));
    if (max_patch_id < 0) {
        return get_snapshot_manager()->get_latest_snapshot(0);
    }
    return max_patch_id;
}

void Controller::cleanup(string basePath, Controller* controller) {
    // Delete patch files
    std::map<int, PatchTree*> patches = controller->get_patch_tree_manager()->get_patch_trees();
    std::map<int, PatchTree*>::iterator itP = patches.begin();
    std::list<int> patchMetadataToDelete;
    while(itP != patches.end()) {
        int id = itP->first;
        std::remove((basePath + PATCHTREE_FILENAME(id, "spo_deletions")).c_str());
        std::remove((basePath + PATCHTREE_FILENAME(id, "spo")).c_str());
        std::remove((basePath + PATCHTREE_FILENAME(id, "pos")).c_str());
        std::remove((basePath + PATCHTREE_FILENAME(id, "pso")).c_str());
        std::remove((basePath + PATCHTREE_FILENAME(id, "sop")).c_str());
        std::remove((basePath + PATCHTREE_FILENAME(id, "osp")).c_str());
        patchMetadataToDelete.push_back(id);
        itP++;
    }

    // Delete snapshot files
    std::map<int, HDT*> snapshots = controller->get_snapshot_manager()->get_snapshots();
    std::map<int, HDT*>::iterator itS = snapshots.begin();
    std::list<int> patchDictsToDelete;
    while(itS != snapshots.end()) {
        int id = itS->first;
        std::remove((basePath + SNAPSHOT_FILENAME_BASE(id)).c_str());
        std::remove((basePath + SNAPSHOT_FILENAME_BASE(id) + ".index").c_str());

        patchDictsToDelete.push_back(id);
        itS++;
    }

    delete controller;

    // Delete dictionaries
    std::list<int>::iterator it1;
    for(it1=patchDictsToDelete.begin(); it1!=patchDictsToDelete.end(); ++it1) {
        DictionaryManager::cleanup(basePath, *it1);
    }

    // Delete metadata files
    std::list<int>::iterator it2;
    for(it2=patchMetadataToDelete.begin(); it2!=patchMetadataToDelete.end(); ++it2) {
        std::remove((basePath + METADATA_FILENAME_BASE(*it2)).c_str());
    }
}

PatchBuilder* Controller::new_patch_bulk() {
    return new PatchBuilder(this);
}

PatchBuilderStreaming *Controller::new_patch_stream() {
    return new PatchBuilderStreaming(this);
}
