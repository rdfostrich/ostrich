#ifndef TPFPATCH_STORE_PATCHTREEMANAGER_H
#define TPFPATCH_STORE_PATCHTREEMANAGER_H

#define PATCHTREE_FILENAME_BASE(id) ("patchtree_" + std::to_string(id) + ".kct")
#define PATCHTREE_FILENAME(id,suffix) (PATCHTREE_FILENAME_BASE(id) + "_" + suffix)

#include <regex>
#include <map>
#include "patch_tree.h"

class PatchTreeManager {
private:
    // Mapping from LOADED patchtree_id -> patch_id
    std::map<int, PatchTree*> loaded_patches;
public:
    PatchTreeManager();
    ~PatchTreeManager();
    /**
     * Add the given patch to a patch tree.
     * @param patch The patch to add.
     * @param patch_id The id of the patch to add.
     * @return If the append succeeded.
     */
    bool append(Patch patch, int patch_id);
    /**
     * Find all patch trees in the current directory.
     * @return The found patch trees
     */
    std::map<int, PatchTree*> detect_patch_trees();
    /**
     * Get the internal patch tree mapping.
     * @return The patch trees
     */
    std::map<int, PatchTree*> get_patch_trees();
    /**
     * Load the corresponding patch tree in memory.
     * @param patch_id_start The id of the patchtree to load, which is the id of the first patch in this tree.
     */
    PatchTree* load_patch_tree(int patch_id_start);
    /**
     * Get a patchtree by id.
     * Calling this will automatically load it (or create it) in memory if it is not present.
     * @param patch_id_start The id of the patchtree to load, which is the id of the first patch in this tree.
     * @param The requested patch tree.
     * @return The found patch tree
     */
    PatchTree* get_patch_tree(int patch_id_start);
    /**
     * Creates a new patch tree.
     * @param patch_id_start The id of the patchtree to load, which is the id of the first patch in this tree.
     * @return The newly created patch tree
     */
    PatchTree* construct_next_patch_tree(int patch_id_start);
    /**
     * Get the patchtree id that contains the given patch id.
     * @param patch_id The id of a patch.
     * @return The id of the patch tree, can be -1 if the patch_id is not present in any tree.
     */
    int get_patch_tree_id(int patch_id);
    /**
     * Get the patch with the given id.
     * @param patch_id The id of a patch.
     * @return The id of a patch.
     */
    Patch get_patch(int patch_id);
};


#endif //TPFPATCH_STORE_PATCHTREEMANAGER_H
