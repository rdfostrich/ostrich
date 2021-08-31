#ifndef TPFPATCH_STORE_CONTROLLER_H
#define TPFPATCH_STORE_CONTROLLER_H

#include "../patch/patch_tree.h"
#include "../snapshot/snapshot_manager.h"
#include "../patch/patch_tree_manager.h"
#include "patch_builder.h"
#include "patch_builder_streaming.h"
#include "triple_delta_iterator.h"
#include "triple_versions_iterator.h"
#include "snapshot_creation_strategy.h"


class Controller {
private:
    PatchTreeManager* patchTreeManager;
    SnapshotManager* snapshotManager;
    SnapshotCreationStrategy* strategy;
    CreationStrategyMetadata* metadata;

public:
    explicit Controller(string basePath, int8_t kc_opts = 0, bool readonly = false);
    Controller(string basePath, SnapshotCreationStrategy* strategy, int8_t kc_opts = 0, bool readonly = false);
    ~Controller();
    /**
     * Get an iterator for all triples matching the given triple pattern with a certain offset
     * in the list of all triples for the given patch id.
     * @param triple_pattern Only triples matching this pattern will be returned.
     * @param offset A certain offset the iterator should start with.
     * @param patch_id The patch id for which triples should be returned.
     */
    TripleIterator* get_version_materialized(const Triple &triple_pattern, int offset, int patch_id) const;
    std::pair<size_t, ResultEstimationType> get_version_materialized_count(const Triple& triple_pattern, int patch_id, bool allowEstimates = false) const;
    size_t get_version_materialized_count_estimated(const Triple& triple_pattern, int patch_id) const;
    /**
     * Get an addition/deletion iterator for all triples matching the given triple pattern with a certain offset
     * in the list of all triples that have been added or removed from patch_id_start until patch_id_end.
     * Triples are annotated with addition or deletion.
     * @param triple_pattern Only triples matching this pattern will be returned.
     * @param offset A certain offset the iterator should start with.
     * @param patch_id The patch id for which triples should be returned.
     */
    TripleDeltaIterator* get_delta_materialized(const Triple &triple_pattern, int offset, int patch_id_start, int patch_id_end) const;
    std::pair<size_t, ResultEstimationType> get_delta_materialized_count(const Triple& triple_pattern, int patch_id_start, int patch_id_end, bool allowEstimates = false) const;
    size_t get_delta_materialized_count_estimated(const Triple& triple_pattern, int patch_id_start, int patch_id_end) const;
    /**
     * Get an iterator for all triples matching the given triple pattern with a certain offset
     * in the list of all triples that exist for any patch id.
     * Triples are annotated with the version in which they are valid.
     * @param triple_pattern Only triples matching this pattern will be returned.
     * @param offset A certain offset the iterator should start with.
     * @param patch_id The patch id for which triples should be returned.
     */
    TripleVersionsIterator* get_version(const Triple &triple_pattern, int offset) const;
    std::pair<size_t, ResultEstimationType> get_version_count(const Triple& triple_pattern, bool allowEstimates = false) const;
    size_t get_version_count_estimated(const Triple& triple_pattern) const;

    /**
     * Add the given patch to a patch tree.
     * @param patch_it The patch iterator with sorted elements to add.
     * @param patch_id The id of the patch to add.
     * @param check_uniqueness If triple uniqueness for the given patch id must be checked, will slow down insertion if true, which is the default behaviour.
     * @param progressListener an optional progress listener.
     * @return If the append succeeded.
     * @note The patch iterator MUST provide triples sorted by SPO.
     */
    bool append(PatchElementIterator* patch_it, int patch_id, DictionaryManager* dict, bool check_uniqueness = true, ProgressListener* progressListener = NULL);
    /**
     * Add the given patch to a patch tree.
     * @param patch The patch to add.
     * @param patch_id The id of the patch to add.
     * @param check_uniqueness If triple uniqueness for the given patch id must be checked, will slow down insertion if true, which is the default behaviour.
     * @param progressListener an optional progress listener.
     * @return If the append succeeded.
     */
    bool append(const PatchSorted& patch, int patch_id, DictionaryManager* dict, bool check_uniqueness = true, ProgressListener* progressListener = NULL);

    /**
     * @return The internal patchtree manager.
     */
    PatchTreeManager* get_patch_tree_manager() const;
    /**
     * @return The internal snapshot manager.
     */
    SnapshotManager* get_snapshot_manager() const;
    /**
     * @return The DictionaryManager file for a certain patch id, this patch id does not have to be created yet.
     */
    DictionaryManager* get_dictionary_manager(int patch_id) const;
    /**
     * @return The largest patch id that is currently available.
     */
    int get_max_patch_id();
    /**
     * @return a new bulk patch builder.
     */
    PatchBuilder* new_patch_bulk();
    /**
     * @return a new streaming patch builder.
     */
    PatchBuilderStreaming* new_patch_stream();
    /**
     * Removes all the files that were created by the controller.
     */
    static void cleanup(string basePath, Controller* controller);

    /**
     * Compute the metadata required for snapshot creation decision
     */
    void init_strategy_metadata();

    /**
    * Add the content from the given files to the patch tree
    * Create new snapshots when relevant, according to the given strategy.
    * @param files The list of files to ingest as a pair of filenames and boolean indicating if it's additions.
    * @param patch_id The id of the patch.
    * @return if ingestion has succeeded
    */
    bool ingest(const std::vector<std::pair<IteratorTripleString*, bool>>& files, int patch_id, ProgressListener* progressListener = nullptr);


};


#endif //TPFPATCH_STORE_CONTROLLER_H
