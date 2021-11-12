#ifndef TPFPATCH_STORE_SNAPSHOT_MANAGER_H
#define TPFPATCH_STORE_SNAPSHOT_MANAGER_H

#define SNAPSHOT_FILENAME_BASE(id) ("snapshot_" + std::to_string(id) + ".hdt")

#include <memory>
#include <HDT.hpp>
#include "../patch/patch.h"
#include <Dictionary.hpp>
#include "../dictionary/dictionary_manager.h"

using namespace hdt;

class SnapshotManager {
private:
    string basePath;

    size_t max_loaded_snapshots;
    std::list<int> lru_list;
    std::map<int, std::list<int>::iterator> lru_map;

    std::map<int, std::shared_ptr<HDT>> loaded_snapshots;
    std::map<int, std::shared_ptr<DictionaryManager>> loaded_dictionaries;
    bool readonly;

    void update_cache_internal(int accessed_id, int iterations);

public:
    explicit SnapshotManager(string basePath, bool readonly = false);
    ~SnapshotManager();
    /**
     * Get the id of the snapshot that is smaller or equal than the given patch id.
     * @param patch_id The patch id to look up.
     * @return The snapshot id. If this is equal to the given patch_id,
     * this means that patch_id is directly referring to a snapshot.
     */
    int get_latest_snapshot(int patch_id);

    int get_max_snapshot_id();

    /**
     * Load the HDT file for the given snapshot id.
     */
    std::shared_ptr<HDT> load_snapshot(int snapshot_id);
    /**
     * Get the HDT file for the given snapshot id.
     */
    std::shared_ptr<HDT> get_snapshot(int snapshot_id);
    /**
     * Create a HDT file for the given snapshot id.
     * It will automatically be persisted in this manager.
     * @param snapshot_id The id for the new snapshot
     * @param triples The stream of triples to create a snapshot from.
     * @param base_uri The base uri for the triples graph.
     * @return The created snapshot
     */
    std::shared_ptr<HDT> create_snapshot(int snapshot_id, IteratorTripleString* triples, string base_uri, ProgressListener* listener = NULL);
    /**
     * Create a HDT file for the given snapshot id.
     * It will automatically be persisted in this manager.
     * @param snapshot_id The id for the new snapshot
     * @param triples_file The RDF file to load triples from.
     * @param base_uri The base uri for the triples graph.
     * @param notation The RDF serialization type of the file.
     * @return The created snapshot
     */
    std::shared_ptr<HDT> create_snapshot(int snapshot_id, string triples_file, string base_uri, RDFNotation notation);
    /**
     * Find all snapshots in the current directory.
     * @return The found patch trees
     */
    const std::map<int, std::shared_ptr<HDT>>& detect_snapshots();
    /**
     * Get the internal snapshot mapping.
     * @return The snapshots
     */
    const std::map<int,  std::shared_ptr<HDT>>& get_snapshots();
    /**
     * Search the given triple pattern in the given hdt file with a certain offset.
     * @param hdt A hdt file
     * @param triple_pattern A triple pattern
     * @param offset The offset the iterator should start from.
     * @return the iterator.
     */
    static IteratorTripleID* search_with_offset(std::shared_ptr<HDT> hdt, const Triple& triple_pattern, long offset);

    /**
     * @return The DictionaryManager file for the given snapshot id.
     */
    std::shared_ptr<DictionaryManager> get_dictionary_manager(int snapshot_id);

    /**
     * Update the state of the patch cache
     */
    void update_cache(int accessed_snapshot_id);
};


#endif //TPFPATCH_STORE_SNAPSHOT_MANAGER_H
