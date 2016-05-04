#ifndef TPFPATCH_STORE_CONTROLLER_H
#define TPFPATCH_STORE_CONTROLLER_H

#include "../patch/patch_tree.h"
#include "../snapshot/snapshot_manager.h"
#include "../patch/patch_tree_manager.h"

class Controller {
private:
    PatchTreeManager* patchTreeManager;
    SnapshotManager* snapshotManager;
public:
    Controller();
    ~Controller();
    TripleIterator* get(Triple triple_pattern, int offset, int patch_id);
    /**
     * Add the given patch to a patch tree.
     * @param patch The patch to add.
     * @param patch_id The id of the patch to add.
     * @return If the append succeeded.
     */
    bool append(Patch patch, int patch_id);
    /**
     * @return The internal patchtree manager.
     */
    PatchTreeManager* get_patch_tree_manager();
    /**
     * @return The internal snapshot manager.
     */
    SnapshotManager* get_snapshot_manager();
};


#endif //TPFPATCH_STORE_CONTROLLER_H
