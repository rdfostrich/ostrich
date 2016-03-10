#ifndef TPFPATCH_STORE_SNAPSHOT_MANAGER_H
#define TPFPATCH_STORE_SNAPSHOT_MANAGER_H

#define SNAPSHOT_FILENAME_BASE(id) ("snapshot_" + std::to_string(id) + ".hdt")

#include <HDT.hpp>
#include "../patch/patch.h"

using namespace hdt;

class SnapshotManager {
private:
    std::map<int, HDT*> loaded_snapshots;
public:
    SnapshotManager();
    /**
     * Get the id of the snapshot that is smaller or equal than the given patch id.
     * @param patch_id The patch id to look up.
     * @return The snapshot id. If this is equal to the given patch_id,
     * this means that patch_id is directly refering to a snapshot.
     */
    int get_latest_snapshot(int patch_id);
    /**
     * Load the HDT file for the given snapshot id.
     */
    HDT* load_snapshot(int snapshot_id);
    /**
     * Get the HDT file for the given snapshot id.
     */
    HDT* get_snapshot(int snapshot_id);
    /**
     * Get the HDT file for the given snapshot id.
     * It will automatically be persisted in this manager.
     * @param snapshot_id The id for the new snapshot
     * @param triples The stream of triples to create a snapshot from.
     * @return The created snapshot
     */
    HDT* create_snapshot(int snapshot_id, IteratorTripleString* triples);
    /**
     * Find all snapshots in the current directory.
     * @return The found patch trees
     */
    std::map<int, HDT*> detect_snapshots();
    /**
     * Get the internal snapshot mapping.
     * @return The snapshots
     */
    std::map<int, HDT*> get_snapshots();
    /**
     * Search the given triple pattern in the given hdt file with a certain offset.
     * @param hdt A hdt file
     * @param triple_pattern A triple pattern
     * @param offset The offset the iterator should start from.
     * @return the iterator.
     */
    static IteratorTripleString* search_with_offset(HDT* hdt, Triple triple_pattern, long offset);
};


#endif //TPFPATCH_STORE_SNAPSHOT_MANAGER_H
