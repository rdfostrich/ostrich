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
    Controller(int8_t kc_opts = 0);
    ~Controller();
    TripleIterator* get(const Triple& triple_pattern, int offset, int patch_id) const;
    /**
     * Add the given patch to a patch tree.
     * @param patch The patch to add.
     * @param patch_id The id of the patch to add.
     * @param progressListener an optional progress listener.
     * @return If the append succeeded.
     */
    bool append(const Patch& patch, int patch_id, DictionaryManager* dict, ProgressListener* progressListener = NULL);
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
     * Removes all the files that were created by the controller.
     */
    static void cleanup(Controller* controller);
};


#endif //TPFPATCH_STORE_CONTROLLER_H
