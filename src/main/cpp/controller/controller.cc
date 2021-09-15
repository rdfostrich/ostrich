#include <util/StopWatch.hpp>
#include "controller.h"
#include "snapshot_patch_iterator_triple_id.h"
#include "patch_builder_streaming.h"
#include "../snapshot/combined_triple_iterator.h"
#include "../simpleprogresslistener.h"
#include <sys/stat.h>

#define BASEURI "<http://example.org>"


Controller::Controller(string basePath, int8_t kc_opts, bool readonly) : Controller(basePath, nullptr, kc_opts,
                                                                                    readonly) {}

Controller::Controller(string basePath, SnapshotCreationStrategy *strategy, int8_t kc_opts, bool readonly)
        : patchTreeManager(new PatchTreeManager(basePath, kc_opts, readonly)),
          snapshotManager(new SnapshotManager(basePath, readonly)),
          strategy(strategy), metadata(nullptr) {
    struct stat sb{};
    if (!(stat(basePath.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode))) {
        throw std::invalid_argument("The provided path '" + basePath + "' is not a valid directory.");
    }

    // Get the metadata for snapshot creation
    init_strategy_metadata();
}

Controller::~Controller() {
    delete patchTreeManager;
    delete snapshotManager;
    delete metadata;
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
    if(patchTree == nullptr) {
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
    if(patchTree == nullptr) {
        return new SnapshotTripleIterator(snapshot_it);
    }
    PositionedTripleIterator* deletion_it = nullptr;
    long added_offset = 0;
    bool check_offseted_deletions = true;

    // Limit the patch id to the latest available patch id
    int max_patch_id = patchTree->get_max_patch_id();
    if (patch_id > max_patch_id) {
        patch_id = max_patch_id;
    }

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
            deletion_it->getPatchTreeIterator()->set_early_break(true);

            // Calculate a new offset, taking into account deletions.
            PositionedTriple first_deletion_triple;
            long snapshot_offset = 0;
            if (deletion_it->next(&first_deletion_triple, true)) {
                snapshot_offset = first_deletion_triple.position;
            } else {
                // The exact snapshot triple could not be found as a deletion
                if (patchTree->get_spo_comparator()->compare(firstTriple, deletion_count_data.second) < 0) {
                    // If the snapshot triple is smaller than the largest deletion,
                    // set the offset to zero, as all deletions will come *after* this triple.

                    // Note that it should impossible that there would exist a deletion *before* this snapshot triple,
                    // otherwise we would already have found this triple as a snapshot triple before.
                    // If we would run into issues because of this after all, we could do a backwards step with
                    // deletion_it and see if we find a triple matching the pattern, and use its position.

                    snapshot_offset = 0;
                } else {
                    // If the snapshot triple is larger than the largest deletion,
                    // set the offset to the total number of deletions.
                    snapshot_offset = deletion_count_data.first;
                }
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
    // TODO: this will require some changes when we support multiple snapshots.
    if (patch_id_start == 0) {
        DictionaryManager *dict = get_snapshot_manager()->get_dictionary_manager(0);
        PatchTree* patchTree = get_patch_tree_manager()->get_patch_tree(0, dict);
        size_t count = patchTree->deletion_count(triple_pattern, patch_id_end).first + patchTree->addition_count(patch_id_end, triple_pattern);
        return std::make_pair(count, EXACT);
    } else {
        if (allowEstimates) {
            DictionaryManager *dict = get_snapshot_manager()->get_dictionary_manager(0);
            PatchTree* patchTree = get_patch_tree_manager()->get_patch_tree(0, dict);
            size_t count_start = patchTree->deletion_count(triple_pattern, patch_id_start).first + patchTree->addition_count(patch_id_start, triple_pattern);
            size_t count_end = patchTree->deletion_count(triple_pattern, patch_id_end).first + patchTree->addition_count(patch_id_end, triple_pattern);
            // There may be an overlap between the delta-triples from start and end.
            // This overlap is not easy to determine, so we ignore it when possible.
            // The real count will never be higher this value, because we should subtract the overlap count.
            return std::make_pair(count_start + count_end, UP_TO);
        } else {
            return std::make_pair(get_delta_materialized(triple_pattern, 0, patch_id_start, patch_id_end)->get_count(), EXACT);
        }
    }
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
            if (TripleStore::is_default_tree(triple_pattern)) {
                return (new ForwardPatchTripleDeltaIterator<PatchTreeDeletionValue>(patchTree, triple_pattern, patch_id_end))->offset(offset);
            } else {
                return (new ForwardPatchTripleDeltaIterator<PatchTreeDeletionValueReduced>(patchTree, triple_pattern, patch_id_end))->offset(offset);
            }
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
            if (TripleStore::is_default_tree(triple_pattern)) {
                return (new FowardDiffPatchTripleDeltaIterator<PatchTreeDeletionValue>(patchTree, triple_pattern, patch_id_start, patch_id_end))->offset(offset);
            } else {
                return (new FowardDiffPatchTripleDeltaIterator<PatchTreeDeletionValueReduced>(patchTree, triple_pattern, patch_id_start, patch_id_end))->offset(offset);
            }
        } else {
            // TODO: implement this when multiple snapshots are supported
            throw std::invalid_argument("Multiple snapshots are not supported.");
        }
    }
    return nullptr;
}

std::pair<size_t, ResultEstimationType> Controller::get_version_count(const Triple &triple_pattern, bool allowEstimates) const {
    // TODO: this will require some changes when we support multiple snapshots.
    // Find the snapshot an count its elements
    HDT* snapshot = get_snapshot_manager()->get_snapshot(0);
    IteratorTripleID* snapshot_it = SnapshotManager::search_with_offset(snapshot, triple_pattern, 0);
    size_t count = snapshot_it->estimatedNumResults();
    if (!allowEstimates && snapshot_it->numResultEstimation() != EXACT) {
        count = 0;
        while (snapshot_it->hasNext()) {
            snapshot_it->next();
            count++;
        }
    }

    // Count the additions for all versions
    DictionaryManager *dict = get_snapshot_manager()->get_dictionary_manager(0);
    PatchTree* patchTree = get_patch_tree_manager()->get_patch_tree(0, dict);
    if (patchTree != NULL) {
        count += patchTree->addition_count(0, triple_pattern);
    }
    return std::make_pair(count, allowEstimates ? snapshot_it->numResultEstimation() : EXACT);
}

std::pair<size_t, ResultEstimationType> Controller::get_version_count(const TemporaryTriple &triple_pattern, bool allowEstimates) const {


    // In a multiple snapshots context, the count has to be EXACT
    // Meaning that we get all triples matching the pattern
    // we remove duplicates (each delta chain will have duplicated triples with respect to other DC)
    // there is probably optimizations opportunities.
    ResultEstimationType estimation_type_used = hdt::EXACT;
    std::set<std::string> triples;
    auto snapshots = snapshotManager->get_snapshots();
    size_t count = 0;
    for (auto snapshot: snapshots) {
        DictionaryManager* dict = snapshotManager->get_dictionary_manager(snapshot.first);
        Triple pattern = triple_pattern.get_as_triple(dict);
        IteratorTripleID *snapshot_it = SnapshotManager::search_with_offset(snapshot.second, pattern, 0);
        while (snapshot_it->hasNext()) {
            Triple t(*snapshot_it->next());
            triples.insert(t.to_string(*dict));
        }

        // Count the additions for all versions
        // We get the ID of the next patch_tree (after the current snapshot)
        int patch_tree_id = patchTreeManager->get_patch_tree_id(snapshot.first+1);;
        if (patch_tree_id > -1) {
            PatchTree* patchTree = patchTreeManager->get_patch_tree(patch_tree_id, dict);
            if (patchTree != nullptr) {
                auto it = patchTree->addition_iterator(pattern);
                Triple t;
                PatchTreeValueBase<PatchTreeDeletionValue> del_val;
                while (it->next(&t, &del_val)) {
                    triples.insert(t.to_string(*dict));
                }
            }
        }
    }
    count = triples.size();
    return std::make_pair(count, estimation_type_used);
}

size_t Controller::get_version_count_estimated(const Triple &triple_pattern) const {
    return get_version_count(triple_pattern, true).first;
}

TripleVersionsIterator* Controller::get_version(const Triple &triple_pattern, int offset) const {
    // TODO: this will require some changes when we support multiple snapshots. (probably just a simple merge for all snapshots with what is already here)
    // Find the snapshot
    int snapshot_id = 0;
    HDT* snapshot = get_snapshot_manager()->get_snapshot(snapshot_id);
    IteratorTripleID* snapshot_it = SnapshotManager::search_with_offset(snapshot, triple_pattern, offset);
    DictionaryManager *dict = get_snapshot_manager()->get_dictionary_manager(snapshot_id);
    PatchTree* patchTree = get_patch_tree_manager()->get_patch_tree(snapshot_id, dict); // Can be null, if only snapshot is available

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

TripleVersionsIteratorCombined *Controller::get_version(const TemporaryTriple &triple_pattern, int offset) const {
    TripleVersionsIteratorCombined* it_version = new TripleVersionsIteratorCombined;

    for (auto it: snapshotManager->get_snapshots()) {
        DictionaryManager* dict = snapshotManager->get_dictionary_manager(it.first);
        Triple pattern = triple_pattern.get_as_triple(dict);
        IteratorTripleID* snapshot_it = SnapshotManager::search_with_offset(it.second, pattern, offset);
        int patch_tree_id = patchTreeManager->get_patch_tree_id(it.first+1);
        PatchTree* patchTree = patchTreeManager->get_patch_tree(patch_tree_id, dict);

        // Snapshots have already been offsetted, calculate the remaining offset.
        // After this, offset will only be >0 if we are past the snapshot elements and at the additions.
        if (snapshot_it->numResultEstimation() == EXACT) {
            offset -= snapshot_it->estimatedNumResults();
            if (offset <= 0) {
                offset = 0;
            } else {
                delete snapshot_it;
                snapshot_it = nullptr;
            }
        } else {
            IteratorTripleID *tmp_it = SnapshotManager::search_with_offset(it.second, pattern, 0);
            while (tmp_it->hasNext() && offset > 0) {
                tmp_it->next();
                offset--;
            }
            delete tmp_it;
        }

        it_version->append_iterator(new TripleVersionsIterator(pattern, snapshot_it, patchTree, it.first), dict);
    }
    // Should we really offset now ?
    return it_version->offset(offset);
}

bool Controller::append(PatchElementIterator* patch_it, int patch_id, DictionaryManager* dict, bool check_uniqueness, ProgressListener* progressListener) {
    // Detect if we need to construct a new patchTree (when last patch triggered a new snapshot)
    int snapshot_id = snapshotManager->get_latest_snapshot(patch_id);
    int patch_tree_id = patchTreeManager->get_patch_tree_id(patch_id);
    if (snapshot_id >= patch_tree_id) {
        patchTreeManager->construct_next_patch_tree(patch_id, dict);
    }

    // Ingest as a regular delta
    bool status = patchTreeManager->append(patch_it, patch_id, dict, check_uniqueness, progressListener);

    // If we need to create a new snapshot:
    // - We do a VM query on the current patch_id
    // - We use the result of the query to make a new snapshot
    // - (optional) we delete the current patch_id in the patch_tree ?
    metadata->patch_id = patch_id;
    bool create_snapshot = strategy != nullptr && strategy->doCreate(*metadata);
    if (create_snapshot) {
        NOTIFYMSG(progressListener, "\nCreating snapshot from patch...\n");
        NOTIFYMSG(progressListener, "\nMaterializing version ...\n");
        TripleIterator* triples_vm = get_version_materialized(Triple("", "", "", dict), 0, patch_id);
        std::vector<TripleString> triples;
        Triple t;
        while (triples_vm->next(&t)) {
            triples.emplace_back(t.get_subject(*dict), t.get_predicate(*dict), t.get_object(*dict));
        }
        IteratorTripleStringVector vec_it(&triples);
        NOTIFYMSG(progressListener, "\nCreating new snapshot ...\n");
        std::cout.setstate(std::ios_base::failbit); // Disable cout info from HDT
        snapshotManager->create_snapshot(patch_id, &vec_it, BASEURI, progressListener);
        std::cout.clear();
    }
//    update_strategy_metadata();
    return status;
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
    get_snapshot_manager()->get_snapshot(snapshot_id); // Force a snapshot load
    return get_snapshot_manager()->get_dictionary_manager(snapshot_id);
}

int Controller::get_max_patch_id() {
    int snapshot_id = get_snapshot_manager()->get_max_snapshot_id();
    get_snapshot_manager()->get_snapshot(snapshot_id); // Make sure our first snapshot is loaded, otherwise KC might get intro trouble while reorganising since it needs the dict for that.
    int max_patch_id = get_patch_tree_manager()->get_max_patch_id(get_snapshot_manager()->get_dictionary_manager(snapshot_id));
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
        std::remove((basePath + PATCHTREE_FILENAME(id, "pos_deletions")).c_str());
        std::remove((basePath + PATCHTREE_FILENAME(id, "pso_deletions")).c_str());
        std::remove((basePath + PATCHTREE_FILENAME(id, "sop_deletions")).c_str());
        std::remove((basePath + PATCHTREE_FILENAME(id, "osp_deletions")).c_str());
        std::remove((basePath + PATCHTREE_FILENAME(id, "spo_additions")).c_str());
        std::remove((basePath + PATCHTREE_FILENAME(id, "pos_additions")).c_str());
        std::remove((basePath + PATCHTREE_FILENAME(id, "pso_additions")).c_str());
        std::remove((basePath + PATCHTREE_FILENAME(id, "sop_additions")).c_str());
        std::remove((basePath + PATCHTREE_FILENAME(id, "osp_additions")).c_str());
        std::remove((basePath + PATCHTREE_FILENAME(id, "count_additions")).c_str());
        std::remove((basePath + PATCHTREE_FILENAME(id, "count_additions.tmp")).c_str());
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
        std::remove((basePath + SNAPSHOT_FILENAME_BASE(id) + ".index.v1-1").c_str());

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

bool Controller::ingest(const std::vector<std::pair<IteratorTripleString *, bool>> &files, int patch_id,
                        ProgressListener *progressListener) {

    DictionaryManager *dict;

    bool first = patch_id == 0;

    // Initialize iterators
    CombinedTripleIterator *it_snapshot;
    PatchElementIteratorCombined *it_patch;
    if (first) {
        it_snapshot = new CombinedTripleIterator();
    } else {
        int snapshot_id = snapshotManager->get_latest_snapshot(metadata->num_version);
        snapshotManager->load_snapshot(snapshot_id);
        dict = snapshotManager->get_dictionary_manager(snapshot_id);
        it_patch = new PatchElementIteratorCombined(PatchTreeKeyComparator(comp_s, comp_p, comp_o, dict));
    }

    for (auto &file: files) {
        if (first) {
            if (!file.second) {
                cerr << "Initial versions can not contain deletions" << endl;
                return false;
            }
            it_snapshot->appendIterator(file.first);
        } else {
            it_patch->appendIterator(new PatchElementIteratorTripleStrings(dict, file.first, file.second));
        }
    }

    size_t added;
    if (first) {
        NOTIFYMSG(progressListener, "\nCreating snapshot...\n");
        std::cout.setstate(std::ios_base::failbit); // Disable cout info from HDT
        HDT *hdt = snapshotManager->create_snapshot(patch_id, it_snapshot, BASEURI, progressListener);
        std::cout.clear();
        added = hdt->getTriples()->getNumberOfElements();
        delete it_snapshot;
    } else {
        NOTIFYMSG(progressListener, "\nAppending patch...\n");
        append(it_patch, patch_id, dict, false, progressListener);
        added = it_patch->getPassed();

        delete it_patch;
    }

    NOTIFYMSG(progressListener,
              ("\nInserted " + to_string(added) + " for version " + to_string(patch_id) + ".\n").c_str());

    return true;
}

void Controller::init_strategy_metadata() {
    metadata = new CreationStrategyMetadata;
    metadata->num_version = get_number_versions();
    metadata->patch_id = -1;
}

int Controller::get_number_versions() {
    int number_versions = snapshotManager->get_max_snapshot_id();
    if (number_versions > -1) {
        snapshotManager->load_snapshot(number_versions);
        int max_patch_id = patchTreeManager->get_max_patch_id(snapshotManager->get_dictionary_manager(number_versions));
        if (max_patch_id > -1) {
            number_versions = max_patch_id;
        }
    }
//    number_versions = number_versions > -1 ? number_versions + 1 : number_versions;
    return number_versions;
}

void Controller::update_strategy_metadata() {
    metadata->num_version = get_number_versions();
    metadata->patch_id = -1;
}

CreationStrategyMetadata *Controller::get_strategy_metadata() {
    return metadata;
}
