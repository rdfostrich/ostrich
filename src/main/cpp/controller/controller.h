#ifndef TPFPATCH_STORE_CONTROLLER_H
#define TPFPATCH_STORE_CONTROLLER_H

#define PATCHTREE_FILENAME(x) ("patchtree_" + std::to_string(x) + ".kch")

#include <map>
#include "../patch/patch_tree.h"

class Controller {
private:
    // Mapping from LOADED patchtree_id -> patch_id
    std::map<int, PatchTree*> loaded_patches;
public:
    Controller();
    ~Controller();
    iterator<std::input_iterator_tag, Triple> get(Triple triple_pattern, int limit, int offset, int patch_id);
    bool append(Patch patch);

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
     */
    PatchTree* get_patch_tree(int patch_id_start);
    /**
     * Creates a new patch tree.
     * @param patch_id_start The id of the patchtree to load, which is the id of the first patch in this tree.
     */
    void construct_next_patch_tree(int patch_id_start);
    /**
     * Get the patchtree id that contains the given patch id.
     * @param patch_id_start The id of the patchtree to load, which is the id of the first patch in this tree.
     * @return The id of the patch tree, can be -1 if the patch_id is not present in any tree.
     */
    int get_patch_tree_id(int patch_id_start);
    /**
     * Write the tree metadata to the METAFILE file
     */
    void write_treemeta();
    /**
     * Read the tree metadata from the METAFILE file
     */
    void read_treemeta();
};


#endif //TPFPATCH_STORE_CONTROLLER_H
