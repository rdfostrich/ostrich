#ifndef TPFPATCH_STORE_PATCHTREEMANAGER_H
#define TPFPATCH_STORE_PATCHTREEMANAGER_H

#include <regex>
#include <map>
#include <list>
#include <memory>
#include "patch_tree.h"

class PatchTreeManager {
private:
    string basePath;

    size_t max_loaded_patches;
    std::list<int> lru_list;
    std::map<int, std::list<int>::iterator> lru_map;
    // Mapping from patchtree_id -> patchTree
    std::map<int, std::shared_ptr<PatchTree>> loaded_patchtrees;
    // Options for KC trees
    int8_t kc_opts;
    bool readonly;

    void update_cache_internal(int accessed_id, int iterations);

public:
    PatchTreeManager(string basePath, int8_t kc_opts = 0, bool readonly = false);
    ~PatchTreeManager();
    /**
     * Add the given patch to a patch tree.
     * @param patch_it The patch iterator with elements to add.
     * @param patch_id The id of the patch to add.
     * @param dict The dictionary that must be used in the patch tree if a new one will be created.
     * @param check_uniqueness If triple uniqueness for the given patch id must be checked, will slow down insertion if true, which is the default behaviour.
     * @param progressListener an optional progress listener.
     * @return If the append succeeded.
     */
    bool append(PatchElementIterator* patch_it, int patch_id, std::shared_ptr<DictionaryManager> dict, bool check_uniqueness = true, ProgressListener* progressListener = NULL);
    /**
     * Add the given patch to a patch tree.
     * @param patch The patch to add.
     * @param patch_id The id of the patch to add.
     * @param dict The dictionary that must be used in the patch tree if a new one will be created.
     * @param check_uniqueness If triple uniqueness for the given patch id must be checked, will slow down insertion if true, which is the default behaviour.
     * @param progressListener an optional progress listener.
     * @return If the append succeeded.
     */
    bool append(const PatchSorted& patch, int patch_id, std::shared_ptr<DictionaryManager> dict, bool check_uniqueness = true, ProgressListener* progressListener = NULL);
    /**
     * Find all patch trees in the current directory.
     * @return The found patch trees
     */
    //std::map<int, PatchTree*> detect_patch_trees() const;
    const std::map<int, std::shared_ptr<PatchTree>>& detect_patch_trees();
    /**
     * Get the internal patch tree mapping.
     * @return The patch trees
     */
    const std::map<int, std::shared_ptr<PatchTree>>& get_patch_trees() const;
    /**
     * Load the corresponding patch tree in memory.
     * @param patch_id_start The id of the patchtree to load, which is the id of the first patch in this tree.
     * @param dict The dictionary that must be used in the patch tree.
     */
    //PatchTree* load_patch_tree(int patch_id_start, DictionaryManager* dict);
    std::shared_ptr<PatchTree> load_patch_tree(int patch_id_start, std::shared_ptr<DictionaryManager> dict);
    /**
     * Get a patchtree by id.
     * Calling this will automatically load it (or create it) in memory if it is not present.
     * @param patch_id_start The id of the patchtree to load, which is the id of the first patch in this tree.
     * @param patch_id_start The requested patch tree.
     * @param dict The dictionary that must be used in the patch tree if a new one will be created.
     * @return The found patch tree
     */
    //PatchTree* get_patch_tree(int patch_id_start, DictionaryManager* dict);
    std::shared_ptr<PatchTree> get_patch_tree(int patch_id_start, std::shared_ptr<DictionaryManager> dict);
    /**
     * Creates a new patch tree.
     * @param patch_id_start The id of the patchtree to load, which is the id of the first patch in this tree.
     * @param dict The dictionary that must be used in the patch tree.
     * @return The newly created patch tree
     */
    //PatchTree* construct_next_patch_tree(int patch_id_start, DictionaryManager* dict);
    std::shared_ptr<PatchTree> construct_next_patch_tree(int patch_id_start, std::shared_ptr<DictionaryManager> dict);
    /**
     * Get the patchtree id that contains the given patch id.
     * @param patch_id The id of a patch.
     * @return The id of the patch tree, can be -1 if the patch_id is not present in any tree.
     */
    int get_patch_tree_id(int patch_id) const;
    /**
     * Get the patch with the given id.
     * @param patch_id The id of a patch.
     * @param dict The dictionary that must be used in the patch tree if a new one will be created.
     * @return The id of a patch.
     */
    Patch* get_patch(int patch_id, std::shared_ptr<DictionaryManager> dict);
    /**
     * @param dict The dictionary that must be used in the patch tree if a new one will be created.
     * @return The largest patch id that is currently available.
     */
    int get_max_patch_id(std::shared_ptr<DictionaryManager> dict);

    /**
     * Update the state of the patch cache
     */
    void update_cache(int accessed_patch_id);

};


#endif //TPFPATCH_STORE_PATCHTREEMANAGER_H
