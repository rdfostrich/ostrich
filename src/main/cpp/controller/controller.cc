#include <util/StopWatch.hpp>
#include "controller.h"
#include "snapshot_patch_iterator_triple_id.h"
#include "../snapshot/combined_triple_iterator.h"
#include "../simpleprogresslistener.h"
#include <sys/stat.h>

#define BASEURI "<http://example.org>"


Controller::Controller(string basePath, int8_t kc_opts, bool readonly, size_t cache_size) : Controller(basePath, nullptr, kc_opts,
                                                                                    readonly, cache_size) {}

Controller::Controller(string basePath, SnapshotCreationStrategy *strategy, int8_t kc_opts, bool readonly, size_t cache_size)
        : patchTreeManager(new PatchTreeManager(basePath, kc_opts, readonly, cache_size)),
          snapshotManager(new SnapshotManager(basePath, readonly, cache_size)),
          strategy(strategy), metadata(nullptr) {
    struct stat sb{};
    if (!(stat(basePath.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode))) {
        throw std::invalid_argument("The provided path '" + basePath + "' is not a valid directory.");
    }

    // Get the metadata for snapshot creation
    init_strategy_metadata();
    metadata_manager = new MetadataManager(basePath);
}

Controller::~Controller() {
    delete patchTreeManager;
    delete snapshotManager;
    delete metadata;
    delete metadata_manager;
}

size_t Controller::get_version_materialized_count_estimated(const Triple& triple_pattern, int patch_id) const {
    return get_version_materialized_count(triple_pattern, patch_id, true).first;
}

std::pair<size_t, ResultEstimationType> Controller::get_version_materialized_count(const Triple& triple_pattern, int patch_id, bool allowEstimates) const {
    std::shared_ptr<DictionaryManager> dict = snapshotManager->get_dictionary_manager(0);
    StringTriple st(triple_pattern.get_subject(*dict), triple_pattern.get_predicate(*dict), triple_pattern.get_object(*dict));
    return get_version_materialized_count(st, patch_id, allowEstimates);
}

std::pair<size_t, ResultEstimationType> Controller::get_version_materialized_count(const StringTriple& triple_pattern, int patch_id, bool allowEstimates) const {
    int snapshot_id = get_snapshot_manager()->get_latest_snapshot(patch_id);
    if(snapshot_id < 0) {
        return std::make_pair(0, EXACT);
    }

    std::shared_ptr<HDT> snapshot = get_snapshot_manager()->get_snapshot(snapshot_id);

    std::shared_ptr<DictionaryManager> dict = get_snapshot_manager()->get_dictionary_manager(snapshot_id);
    Triple pattern = triple_pattern.get_as_triple(dict);

    IteratorTripleID* snapshot_it = SnapshotManager::search_with_offset(snapshot, pattern, 0);
    size_t snapshot_count = snapshot_it->estimatedNumResults();
    ResultEstimationType res_type = snapshot_it->numResultEstimation();

    if (!allowEstimates && res_type != EXACT) {
        snapshot_count = 0;
        while (snapshot_it->hasNext()) {
            snapshot_it->next();
            snapshot_count++;
        }
    }
    delete snapshot_it;
    if(snapshot_id == patch_id) {
        return std::make_pair(snapshot_count, res_type);
    }

    int id = get_patch_tree_manager()->get_patch_tree_id(patch_id);
    std::shared_ptr<PatchTree> patchTree = get_patch_tree_manager()->get_patch_tree(id, dict);
    if(patchTree == nullptr) {
        return std::make_pair(snapshot_count, res_type);
    }

    std::pair<PatchPosition, Triple> deletion_count_data = patchTree->deletion_count(pattern, patch_id);
    size_t addition_count = patchTree->addition_count(patch_id, pattern);
    return std::make_pair(snapshot_count - deletion_count_data.first + addition_count, res_type);
}

TripleIterator* Controller::get_version_materialized(const Triple &triple_pattern, int offset, int patch_id) const {
    std::shared_ptr<DictionaryManager> dict = snapshotManager->get_dictionary_manager(0);
    StringTriple st(triple_pattern.get_subject(*dict), triple_pattern.get_predicate(*dict), triple_pattern.get_object(*dict));
    return get_version_materialized(st, offset, patch_id);
}

TripleIterator* Controller::get_version_materialized(const StringTriple &triple_pattern, int offset, int patch_id) const {
    // Find the snapshot
    int snapshot_id = get_snapshot_manager()->get_latest_snapshot(patch_id);
    if(snapshot_id < 0) {
        //throw std::invalid_argument("No snapshot was found for version " + std::to_string(patch_id));
        return new EmptyTripleIterator();
    }
    std::shared_ptr<HDT> snapshot = get_snapshot_manager()->get_snapshot(snapshot_id);
    std::shared_ptr<DictionaryManager> dict = get_snapshot_manager()->get_dictionary_manager(snapshot_id);

    Triple pattern = triple_pattern.get_as_triple(dict);

    // Simple case: We are requesting a snapshot, delegate lookup to that snapshot.
    IteratorTripleID* snapshot_it = SnapshotManager::search_with_offset(snapshot, pattern, offset);
    if(snapshot_id == patch_id) {
        return new SnapshotTripleIterator(snapshot_it);
    }

    // Otherwise, we have to prepare an iterator for a certain patch
    int id = get_patch_tree_manager()->get_patch_tree_id(patch_id);
    std::shared_ptr<PatchTree> patchTree = get_patch_tree_manager()->get_patch_tree(id, dict);
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

    std::pair<PatchPosition, Triple> deletion_count_data = patchTree->deletion_count(pattern, patch_id);
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
            deletion_it = patchTree->deletion_iterator_from(firstTriple, patch_id, pattern);
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
            snapshot_it = SnapshotManager::search_with_offset(snapshot, pattern, offset + added_offset);

            // Check if we need to loop again
            check_offseted_deletions = previous_added_offset < added_offset;
            if(check_offseted_deletions) {
                delete deletion_it;
                deletion_it = nullptr;
            }
        } else {
            check_offseted_deletions = false;
        }
    }
    return new SnapshotPatchIteratorTripleID(snapshot_it, deletion_it, patchTree->get_spo_comparator(), snapshot, pattern, patchTree, patch_id, offset, deletion_count_data.first);
}

std::pair<size_t, ResultEstimationType> Controller::get_delta_materialized_count(const Triple &triple_pattern, int patch_id_start, int patch_id_end, bool allowEstimates) const {
    std::shared_ptr<DictionaryManager> dict = snapshotManager->get_dictionary_manager(0);
    StringTriple st(triple_pattern.get_subject(*dict), triple_pattern.get_predicate(*dict), triple_pattern.get_object(*dict));
    return get_delta_materialized_count(st, patch_id_start, patch_id_end, allowEstimates);
}

std::pair<size_t, ResultEstimationType> Controller::get_delta_materialized_count(const StringTriple &triple_pattern, int patch_id_start, int patch_id_end, bool allowEstimates) const {
    if (allowEstimates) {
        int snapshot_id_start = get_snapshot_manager()->get_latest_snapshot(patch_id_start);
        int snapshot_id_end = get_snapshot_manager()->get_latest_snapshot(patch_id_end);
        std::vector<int> snapshots_ids;
        {
            std::map<int, std::shared_ptr<HDT>> snapshots = snapshotManager->get_snapshots();
            auto it1 = snapshots.find(snapshot_id_start);
            auto it2 = snapshots.find(snapshot_id_end);
            snapshots_ids.push_back(it1->first);
            while (it1 != it2) {
                it1++;
                snapshots_ids.push_back(it1->first);
            }
        }
        size_t count = 0;
        // the patch_id_start is a patch not a snapshot
        if (patch_id_start > snapshot_id_start) {
            int id = patchTreeManager->get_patch_tree_id(patch_id_start);
            std::shared_ptr<DictionaryManager> dict = snapshotManager->get_dictionary_manager(snapshot_id_start);
            std::shared_ptr<PatchTree> pt = patchTreeManager->get_patch_tree(id, dict);
            Triple tp = triple_pattern.get_as_triple(dict);
            count += pt->deletion_count(tp, patch_id_start).first + pt->addition_count(patch_id_start, tp);
        }
        // We count for intermediary delta chains
        for (int i = 1; i < snapshots_ids.size(); i++) {
            int id = patchTreeManager->get_patch_tree_id(snapshots_ids[i]);
            std::shared_ptr<DictionaryManager> dict = snapshotManager->get_dictionary_manager(snapshots_ids[i-1]);
            std::shared_ptr<PatchTree> pt = patchTreeManager->get_patch_tree(id, dict);
            Triple tp = triple_pattern.get_as_triple(dict);
            count += pt->deletion_count(tp, snapshots_ids[i]).first + pt->addition_count(snapshots_ids[i], tp);
        }
        // the patch_id_end is a patch not a snapshot
        if (patch_id_end > snapshot_id_end) {
            int id = patchTreeManager->get_patch_tree_id(patch_id_end);
            std::shared_ptr<DictionaryManager> dict = snapshotManager->get_dictionary_manager(snapshot_id_end);
            std::shared_ptr<PatchTree> pt = patchTreeManager->get_patch_tree(id, dict);
            Triple tp = triple_pattern.get_as_triple(dict);
            count += pt->deletion_count(tp, patch_id_end).first + pt->addition_count(patch_id_end, tp);
        }
        return std::make_pair(count, UP_TO);
    }
    return std::make_pair(get_delta_materialized(triple_pattern, 0, patch_id_start, patch_id_end)->get_count(), EXACT);
}

size_t Controller::get_delta_materialized_count_estimated(const Triple &triple_pattern, int patch_id_start, int patch_id_end) const {
    return get_delta_materialized_count(triple_pattern, patch_id_start, patch_id_end, true).second;
}

TripleDeltaIterator* Controller::get_delta_materialized(const Triple &triple_pattern, int offset, int patch_id_start,
                                                        int patch_id_end) const {
    std::shared_ptr<DictionaryManager> dict = snapshotManager->get_dictionary_manager(0);
    StringTriple st(triple_pattern.get_subject(*dict), triple_pattern.get_predicate(*dict), triple_pattern.get_object(*dict));
    return get_delta_materialized(st, offset, patch_id_start, patch_id_end);
}

TripleDeltaIterator* Controller::get_delta_materialized(const StringTriple &triple_pattern, int offset, int patch_id_start,
                                                        int patch_id_end, bool use_plain_diff) const {

    auto single_delta_query = [this, triple_pattern](int start_id, int end_id, std::shared_ptr<DictionaryManager> dict, bool sort = false) {
        TripleDeltaIterator* return_it;
        int patch_tree_id = patchTreeManager->get_patch_tree_id(end_id);
        std::shared_ptr<PatchTree> patch_tree = patchTreeManager->get_patch_tree(patch_tree_id, dict);
        Triple tp = triple_pattern.get_as_triple(dict);
        if(patch_tree == nullptr) {
            return_it = new EmptyTripleDeltaIterator();
        } else {
            int snapshot_id = snapshotManager->get_latest_snapshot(start_id);
            // start_id = patch
            if (start_id != snapshot_id) {
                if (TripleStore::is_default_tree(tp)) {
                    return_it = new FowardDiffPatchTripleDeltaIterator<PatchTreeDeletionValue>(patch_tree, tp, start_id, end_id, dict);
                } else {
                    TripleDeltaIterator* tmp_it = new FowardDiffPatchTripleDeltaIterator<PatchTreeDeletionValueReduced>(patch_tree, tp, start_id, end_id, dict);
                    if (sort) {
                        return_it = new SortedTripleDeltaIterator(tmp_it, SPO);
                    } else {
                        return_it = tmp_it;
                    }
                }
            // start_id = snapshot
            } else {
                if (TripleStore::is_default_tree(tp)) {
                    return_it = new ForwardPatchTripleDeltaIterator<PatchTreeDeletionValue>(patch_tree, tp, end_id, dict);
                } else {
                    TripleDeltaIterator* tmp_it = new ForwardPatchTripleDeltaIterator<PatchTreeDeletionValueReduced>(patch_tree, tp, end_id, dict);
                    if (sort) {
                        return_it = new SortedTripleDeltaIterator(tmp_it, SPO);
                    } else {
                        return_it = tmp_it;
                    }
                }
            }
        }
        return return_it;
    };

    if (patch_id_end <= patch_id_start) {
        return new EmptyTripleDeltaIterator();
    }

    // Find the snapshots
    int snapshot_id_start = snapshotManager->get_latest_snapshot(patch_id_start);
    int snapshot_id_end = snapshotManager->get_latest_snapshot(patch_id_end);
    if (snapshot_id_start < 0 || snapshot_id_end < 0) {
        return new EmptyTripleDeltaIterator();
    }

    // Get the dictionaries
    std::shared_ptr<DictionaryManager> dict_start = snapshotManager->get_dictionary_manager(snapshot_id_start);
    std::shared_ptr<DictionaryManager> dict_end = snapshotManager->get_dictionary_manager(snapshot_id_end);

    // Both patches are in the same delta chain
    if (snapshot_id_start == snapshot_id_end) {
        return (single_delta_query(patch_id_start, patch_id_end, dict_end))->offset(offset);
    }

    if (use_plain_diff) {
        TripleIterator* it1 = get_version_materialized(triple_pattern, 0, patch_id_start);
        TripleIterator* it2 = get_version_materialized(triple_pattern, 0, patch_id_end);
        return (new PlainDiffDeltaIterator(it1, it2, dict_start, dict_end))->offset(offset);
    }

    auto snapshot_diff_it = new AutoSnapshotDiffIterator(triple_pattern, snapshotManager, patchTreeManager, snapshot_id_start, snapshot_id_end);
    TripleDeltaIterator* delta_it_end = nullptr;
    TripleDeltaIterator* intermediate_it = nullptr;

    // start = snapshot and end = snapshot
    if (patch_id_start == snapshot_id_start && patch_id_end == snapshot_id_end) {
        return snapshot_diff_it->offset(offset);
    }

    // start = patch
    if (patch_id_start != snapshot_id_start) {
        TripleDeltaIterator* delta_it_start = single_delta_query(snapshot_id_start, patch_id_start, dict_start, true);
        intermediate_it = new MergeDiffIteratorCase2(delta_it_start, snapshot_diff_it);
    }
    // end = patch
    if (patch_id_end != snapshot_id_end) {
        delta_it_end = single_delta_query(snapshot_id_end, patch_id_end, dict_end, true);
    }

    if (intermediate_it) {
        if (patch_id_end != snapshot_id_end) {
            return (new MergeDiffIterator(intermediate_it, delta_it_end))->offset(offset);
        }
        return intermediate_it->offset(offset);
    }
    return (new MergeDiffIterator(snapshot_diff_it, delta_it_end))->offset(offset);
}

std::pair<size_t, ResultEstimationType> Controller::get_version_count(const Triple &triple_pattern, bool allowEstimates) const {
    std::shared_ptr<DictionaryManager> dict = snapshotManager->get_dictionary_manager(0);
    StringTriple st(triple_pattern.get_subject(*dict), triple_pattern.get_predicate(*dict), triple_pattern.get_object(*dict));
    return get_version_count(st, allowEstimates);
}

std::pair<size_t, ResultEstimationType> Controller::get_version_count(const StringTriple &triple_pattern, bool allowEstimates) const {

    // If allowEstimate is true, when multiple snapshots exists, the count can overestimate the number of results.
    // This is due to triples being duplicated in multiple delta chains that can not be filtered out when only doing estimates.
    ResultEstimationType estimation_type_used = hdt::EXACT;
    size_t count = 0;
    auto snapshots = snapshotManager->get_snapshots();
    if (!allowEstimates && snapshots.size() > 1) {
        TripleVersionsIterator* it = get_version(triple_pattern, 0);
        count = it->get_count();
        delete it;
    } else {
        for (const auto& snapshot: snapshots) {
            std::shared_ptr<DictionaryManager> dict = snapshotManager->get_dictionary_manager(snapshot.first);
            Triple pattern = triple_pattern.get_as_triple(dict);
            std::shared_ptr<HDT> hdt = snapshotManager->get_snapshot(snapshot.first);  // by using get_snapshot, we make sure it's loaded before use
            IteratorTripleID *snapshot_it = SnapshotManager::search_with_offset(hdt, pattern, 0);
            size_t c = snapshot_it->estimatedNumResults();
            ResultEstimationType tmp_est_type = snapshot_it->numResultEstimation();
            if (!allowEstimates) {
                while (snapshot_it->hasNext()) {
                    TripleID* t = snapshot_it->next();
                    count++;
                }
            } else {
                count += c;
                if (tmp_est_type != hdt::EXACT)
                    estimation_type_used = hdt::APPROXIMATE;
            }
            delete snapshot_it;

            // Count the additions for all versions
            // We get the ID of the next patch_tree (after the current snapshot)
            int patch_tree_id = patchTreeManager->get_patch_tree_id(snapshot.first+1);;
            if (patch_tree_id > -1) {
                std::shared_ptr<PatchTree> patchTree = patchTreeManager->get_patch_tree(patch_tree_id, dict);
                if (patchTree != nullptr) {
                    if (allowEstimates) {
                        count += patchTree->addition_count(snapshot.first, pattern);
                    } else {
                        auto it = patchTree->addition_iterator(pattern);
                        Triple t;
                        PatchTreeValueBase<PatchTreeDeletionValue> del_val;
                        while (it->next(&t, &del_val)) {
                            count++;
                        }
                        delete it;
                    }
                }
            }
        }
    }
    return std::make_pair(count, estimation_type_used);
}

size_t Controller::get_version_count_estimated(const Triple &triple_pattern) const {
    return get_version_count(triple_pattern, true).first;
}

TripleVersionsIterator* Controller::get_version(const Triple &triple_pattern, int offset) const {
    std::shared_ptr<DictionaryManager> dict = snapshotManager->get_dictionary_manager(0);
    StringTriple st(triple_pattern.get_subject(*dict), triple_pattern.get_predicate(*dict), triple_pattern.get_object(*dict));
    return get_version(st, offset);
}

TripleVersionsIterator *Controller::get_version(const StringTriple &triple_pattern, int offset) const {
    std::vector<int> snapshots_id;
    for (const auto& it: snapshotManager->get_snapshots()) {
        snapshots_id.push_back(it.first);
    }
    auto it_version = new TripleVersionsIteratorCombined(SPO);
    for (int id: snapshots_id) {
        std::shared_ptr<DictionaryManager> dict = snapshotManager->get_dictionary_manager(id);
        Triple pattern = triple_pattern.get_as_triple(dict);
        std::shared_ptr<HDT> snapshot = snapshotManager->get_snapshot(id);
        IteratorTripleID* snapshot_it = SnapshotManager::search_with_offset(snapshot, pattern, 0);
        int patch_tree_id = patchTreeManager->get_patch_tree_id(id+1);
        std::shared_ptr<PatchTree> patchTree = patchTreeManager->get_patch_tree(patch_tree_id, dict);
        auto it = new PatchTreeTripleVersionsIterator(pattern, snapshot_it, patchTree, id, dict);
        it_version->add_iterator(it);
        delete it;
    }
    return it_version->offset(offset);
}


bool Controller::append(PatchElementIterator* patch_it, int patch_id, std::shared_ptr<DictionaryManager> dict, bool check_uniqueness, ProgressListener* progressListener) {
    // Detect if we need to construct a new patchTree (when last patch triggered a new snapshot)
    int snapshot_id = snapshotManager->get_latest_snapshot(patch_id);
    int patch_tree_id = patchTreeManager->get_patch_tree_id(patch_id);
    if (snapshot_id >= patch_tree_id) {
        patchTreeManager->construct_next_patch_tree(patch_id, dict);
        patch_tree_id = patchTreeManager->get_patch_tree_id(patch_id);
    }

    // Ingest as a regular delta
    auto istart = std::chrono::high_resolution_clock::now();
    bool status = patchTreeManager->append(patch_it, patch_id, dict, check_uniqueness, progressListener);
    auto istop = std::chrono::high_resolution_clock::now();
    auto iduration = std::chrono::duration_cast<std::chrono::nanoseconds>(istop - istart);
    metadata->ingestion_times = metadata_manager->store_uint64("ingest-time", snapshot_id, iduration.count());

    std::shared_ptr<PatchTree> pt = patchTreeManager->get_patch_tree(patch_tree_id, dict);
    Triple tp("", "", "", dict);

    // Fill the metadata struct for strategy
    metadata->patch_id = patch_id;
    metadata->delta_sizes = metadata_manager->store_uint64("delta-size", snapshot_id, patch_it->getPassed());
    size_t add_count = pt->addition_count(patch_id, tp);
    size_t del_count = pt->deletion_count(tp, patch_id).first;
    metadata->agg_delta_sizes = metadata_manager->store_uint64("agg-delta-size", snapshot_id, add_count + del_count);
    metadata->last_snapshot_size = get_version_materialized_count(tp, snapshot_id, true).first;
    metadata->current_version_size = metadata->last_snapshot_size - del_count + add_count;
    double ag = add_count + del_count;
    double su = metadata->last_snapshot_size + ag;
    double change_ratio = ag/su;
    metadata->change_ratios = metadata_manager->store_double("change-ratio", snapshot_id, change_ratio);

    // If we need to create a new snapshot:
    // - We do a VM query on the current patch_id
    // - We use the result of the query to make a new snapshot
    // - (optional) we delete the current patch_id in the patch_tree ?
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
        delete triples_vm;
        IteratorTripleStringVector vec_it(&triples);
        NOTIFYMSG(progressListener, "\nCreating new snapshot ...\n");
        std::cout.setstate(std::ios_base::failbit); // Disable cout info from HDT
        snapshotManager->create_snapshot(patch_id, &vec_it, BASEURI, progressListener);
        std::cout.clear();
    }
    return status;
}

bool Controller::append(const PatchSorted& patch, int patch_id, std::shared_ptr<DictionaryManager> dict, bool check_uniqueness,
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

std::shared_ptr<DictionaryManager> Controller::get_dictionary_manager(int patch_id) const {
    int snapshot_id = get_snapshot_manager()->get_latest_snapshot(patch_id);
    if(snapshot_id < 0) {
        throw std::invalid_argument("No snapshot has been created yet.");
    }
    get_snapshot_manager()->get_snapshot(snapshot_id); // Force a snapshot load
    return get_snapshot_manager()->get_dictionary_manager(snapshot_id);
}

int Controller::get_max_patch_id() const {
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
    std::map<int, std::shared_ptr<PatchTree>> patches = controller->get_patch_tree_manager()->get_patch_trees();
    auto itP = patches.begin();
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
    std::map<int, std::shared_ptr<HDT>> snapshots = controller->get_snapshot_manager()->get_snapshots();
    auto itS = snapshots.begin();
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

    // Delete strategy metadata database
    std::remove((basePath + "ingestion_metadata.kch").c_str());
}

PatchBuilder* Controller::new_patch_bulk() {
    return new PatchBuilder(this);
}

PatchBuilderStreaming *Controller::new_patch_stream() {
    return new PatchBuilderStreaming(this);
}

bool Controller::ingest(const std::vector<std::pair<IteratorTripleString *, bool>> &files, int patch_id,
                        ProgressListener *progressListener) {

    std::shared_ptr<DictionaryManager> dict;

    bool first = patch_id == 0;

    // Initialize iterators
    CombinedTripleIterator *it_snapshot = nullptr;
    PatchElementIteratorCombined *it_patch = nullptr;
    if (first) {
        it_snapshot = new CombinedTripleIterator();
    } else {
        int snapshot_id = snapshotManager->get_latest_snapshot(patch_id);
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
        auto istart = std::chrono::high_resolution_clock::now();
        std::shared_ptr<HDT> hdt = snapshotManager->create_snapshot(patch_id, it_snapshot, BASEURI, progressListener);
        auto istop = std::chrono::high_resolution_clock::now();
        auto iduration = std::chrono::duration_cast<std::chrono::nanoseconds>(istop - istart);
        metadata_manager->store_uint64("ingest-time", patch_id, iduration.count());
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
//    metadata->num_version = get_number_versions();
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
//    metadata->num_version = get_number_versions();
    metadata->patch_id = -1;
}

CreationStrategyMetadata *Controller::get_strategy_metadata() {
    return metadata;
}
