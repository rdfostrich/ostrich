#ifndef TPFPATCH_STORE_SNAPSHOT_MANAGER_H
#define TPFPATCH_STORE_SNAPSHOT_MANAGER_H

#define SNAPSHOT_FILENAME_BASE(id) ("snapshot_" + std::to_string(id) + ".hdt")

#include <memory>
#include <HDT.hpp>
#include "../patch/patch.h"
#include <Dictionary.hpp>
#include "../dictionary/dictionary_manager.h"


class SnapshotManager {
private:
    std::string basePath;

    size_t max_loaded_snapshots;
    std::list<int> lru_list;
    std::map<int, std::list<int>::iterator> lru_map;

    std::map<int, std::shared_ptr<hdt::HDT>> loaded_snapshots;
    std::map<int, std::shared_ptr<DictionaryManager>> loaded_dictionaries;
    bool readonly;

    void update_cache_internal(int accessed_id, int iterations);

public:
    explicit SnapshotManager(string basePath, bool readonly = false, size_t cache_size = 4);
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
    std::shared_ptr<hdt::HDT> load_snapshot(int snapshot_id);
    /**
     * Get the HDT file for the given snapshot id.
     */
    std::shared_ptr<hdt::HDT> get_snapshot(int snapshot_id);
    /**
     * Create a HDT file for the given snapshot id.
     * It will automatically be persisted in this manager.
     * @param snapshot_id The id for the new snapshot
     * @param triples The stream of triples to create a snapshot from.
     * @param base_uri The base uri for the triples graph.
     * @return The created snapshot
     */
    std::shared_ptr<hdt::HDT> create_snapshot(int snapshot_id, hdt::IteratorTripleString* triples, string base_uri, hdt::ProgressListener* listener = NULL);
    /**
     * Create a HDT file for the given snapshot id.
     * It will automatically be persisted in this manager.
     * @param snapshot_id The id for the new snapshot
     * @param triples_file The RDF file to load triples from.
     * @param base_uri The base uri for the triples graph.
     * @param notation The RDF serialization type of the file.
     * @return The created snapshot
     */
    std::shared_ptr<hdt::HDT> create_snapshot(int snapshot_id, string triples_file, string base_uri, hdt::RDFNotation notation);
    /**
     * Find all snapshots in the current directory.
     * @return The found patch trees
     */
    const std::map<int, std::shared_ptr<hdt::HDT>>& detect_snapshots();

    /**
     * Get the ids of the available snapshots
     * @return the ids
     */
    std::vector<int> get_snapshots_ids() const;

    /**
     * Search the given triple pattern in the given hdt file with a certain offset.
     * @param hdt A hdt file
     * @param triple_pattern A triple pattern
     * @param offset The offset the iterator should start from.
     * @param dict optional dict to help translating triple_pattern to HDT with correct ID (i.e. use max ID when unknown by HDT)
     * @return the iterator.
     */
    static hdt::IteratorTripleID* search_with_offset(std::shared_ptr<hdt::HDT> hdt, const Triple& triple_pattern, long offset, std::shared_ptr<DictionaryManager> dict = nullptr);

    /**
     * @return The DictionaryManager file for the given snapshot id.
     */
    std::shared_ptr<DictionaryManager> get_dictionary_manager(int snapshot_id);

    /**
     * Update the state of the patch cache
     */
    void update_cache(int accessed_snapshot_id);

    void set_cache_max_size(size_t new_size);

};


#endif //TPFPATCH_STORE_SNAPSHOT_MANAGER_H
