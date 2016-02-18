#ifndef TPFPATCH_STORE_CONTROLLER_H
#define TPFPATCH_STORE_CONTROLLER_H


#include "../patch/patch_tree.h"

class Controller {
private:
    // Mapping from LOADED patchtree_id -> patch_id
    map<int, PatchTree> loaded_patches;
protected:
    /**
     * Load the corresponding patch in memory.
     * @param patchtree_id The id of the patchtree to load
     */
    void load_patch(int patchtree_id);
    /**
     * Get a patchtree by id.
     * Calling this will automatically load it (or create it) in memory if it is not present.
     * @param patchtree_id The id of the patchtree to load
     * @param The requested patch tree.
     */
    PatchTree get_patch_tree(int patchtree_id);
    /**
     * Creates a new patch tree.
     * @return The id of the newly created patch tree.
     */
    int construct_next_patch_tree();
    /**
     * Get the patchtree id that contains the given patch id.
     * @param patch_id The patch id to find its patchtree for.
     * @return The id of the patch tree, can be -1 if the patch_id is not present in any tree.
     */
    int get_patch_tree_id(int patch_id);
public:
    Controller();
    ~Controller();
    iterator<std::input_iterator_tag, Triple> get(Triple triple_pattern, int limit, int offset, int patch_id);
    bool append(Patch patch);
};


#endif //TPFPATCH_STORE_CONTROLLER_H
