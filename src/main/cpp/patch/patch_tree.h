#ifndef TPFPATCH_STORE_PATCH_TREE_H
#define TPFPATCH_STORE_PATCH_TREE_H

#include <string>
#include <kchashdb.h>
#include "patch_tree_iterator.h"
#include "patch.h"
#include "patch.h"
#include "patch_tree_key_comparator.h"
#include "positioned_triple_iterator.h"
#include "triple_store.h"
#include "triple_iterator.h"
#include "patch_element_iterator.h"

#define PATCHTREE_FILENAME_BASE(id) ("patchtree_" + std::to_string(id) + ".kct")
#define PATCHTREE_FILENAME(id,suffix) (PATCHTREE_FILENAME_BASE(id) + "_" + suffix)
#define METADATA_FILENAME_BASE(id) ("meta_" + std::to_string(id) + ".dat")
// The size of the triple parser buffer during patch insertion.
#ifndef PATCH_INSERT_BUFFER_SIZE
#define PATCH_INSERT_BUFFER_SIZE 100
#endif


// A PatchTree can store Patches which are persisted to a file
class PatchTree {
private:
    TripleStore* tripleStore;
    std::string metadata_filename;
    int min_patch_id;
    int max_patch_id;
    bool readonly;

    kyotocabinet::HashDB sp_;
    kyotocabinet::HashDB s_o;
    kyotocabinet::HashDB s__;
    kyotocabinet::HashDB _po;
    kyotocabinet::HashDB _p_;
    kyotocabinet::HashDB __o;
protected:
    /**
     * Reconstruct the given patch id in the given patch.
     * It will loop over the tree and rebuild the patch.
     * @param patch_id The patch id
     * @param ignore_local_changes If local changes should be ignored when reconstructing the patch, false by default.
     */
    void reconstruct_to_patch(Patch* patch, int patch_id, bool ignore_local_changes = false) const;
    void clear_temp_insertion_trees();
    template <class DV>
    std::pair<DV*, Triple> last_deletion_value(const Triple &triple_pattern, int patch_id) const;
public:
    PatchTree(string basePath, int min_patch_id, std::shared_ptr<DictionaryManager> dict, int8_t kc_opts = 0, bool readonly = false);
    ~PatchTree();
    /**
     * Append the given patch elements to the tree with given patch id.
     * This can OVERWRITE existing elements without a warning.
     * @param patch_it A patch iterator with sorted (SPO) elements
     * @param patch_id The id of the patch
     * @param progressListener an optional progress listener.
     * @note If an error occurs, some elements might have already been added.
     * If you want to change this behaviour, you'll have to first check if the patch elements are really new.
     */
    void append_unsafe(PatchElementIterator *patch_it, int patch_id, hdt::ProgressListener *progressListener = nullptr);
    /**
     * Append the given patch elements to the tree with given patch id.
     * This safe append will first check if the patch is completely new, only then it will add the data
     * @param patch_it The patch elements to add (sorted SPO)
     * @param patch_id The id of the patch
     * @param progressListener an optional progress listener.
     * @return If the patch was added, otherwise the patch was not completely new.
     */
    bool append(PatchElementIterator* patch_it, int patch_id, hdt::ProgressListener* progressListener = nullptr);
    /**
     * Append the given patch elements to the tree with given patch id.
     * This safe append will first check if the patch is completely new, only then it will add the data
     * @param patch The patch to add
     * @param patch_id The id of the patch
     * @param progressListener an optional progress listener.
     * @return If the patch was added, otherwise the patch was not completely new.
     */
    bool append(const PatchSorted& patch, int patch_id, hdt::ProgressListener* progressListener = nullptr);
    /**
     * Check if the given patch element is present in the tree.
     * @param patch_element The patch element to look for
     * @param patch_id The id of the patch to look for
     * @param ignore_type If the element type (addition/deletion) should be ignored.
     * @return If the patch is present in the tree.
     */
    bool contains(const PatchElement& patch_element, int patch_id, bool ignore_type) const;
    /**
     * Check if the given patch element is present in the addition tree.
     * @param patch_element The patch element to look for
     * @param patch_id The id of the patch to look for
     * @return If the patch is present in the addition tree.
     */
    bool contains_addition(const PatchElement& patch_element, int patch_id) const;
    /**
     * Check if the given patch element is present in the deletion tree.
     * @param patch_element The patch element to look for
     * @param patch_id The id of the patch to look for
     * @return If the patch is present in the deletion tree.
     */
    bool contains_deletion(const PatchElement& patch_element, int patch_id) const;
    /**
     * Reconstruct a hashed patch based on the given patch id.
     * It will loop over the tree and rebuild the patch.
     * @param patch_id The patch id
     * @param ignore_local_changes If local changes should be ignored when reconstructing the patch, false by default.
     * @return The reconstructed patch
     */
    PatchHashed* reconstruct_patch_hashed(int patch_id, bool ignore_local_changes = false) const;
    /**
     * Reconstruct a sorted patch based on the given patch id.
     * It will loop over the tree and rebuild the patch.
     * @param patch_id The patch id
     * @param ignore_local_changes If local changes should be ignored when reconstructing the patch, false by default.
     * @return The reconstructed patch
     */
    PatchSorted* reconstruct_patch(int patch_id, bool ignore_local_changes = false) const;
    /**
     * Get an iterator starting from the given key.
     * @param key The key to start from
     * @return The iterator that will loop over the tree from the given key.
     */
    PatchTreeIterator iterator(PatchTreeKey* key) const;
    /**
     * Get an iterator starting from the start of the tree and only emitting the elements in the given patch.
     * @param patch_id The patch id to filter by, this includes all patches before this one.
     * @param exact If only patches exactly matching the given id should be returned,
     *              otherwise all patches with an id <= patch_id will be returned.
     * @return The iterator that will loop over the tree for the given patch.
     */
    PatchTreeIterator iterator(int patch_id, bool exact) const;
    /**
     * Get an iterator starting from the given key and only emitting the elements in the given patch.
     * @param key The key to start from
     * @param patch_id The patch id to filter by, this includes all patches before this one.
     * @param exact If only patches exactly matching the given id should be returned,
     *              otherwise all patches with an id <= patch_id will be returned.
     * @return The iterator that will loop over the tree for the given patch.
     */
    PatchTreeIterator iterator(PatchTreeKey* key, int patch_id, bool exact) const;
    /**
     * Get an iterator starting for the given triple_pattern and only emitting the elements in the given patch.
     * @param triple_pattern The triple pattern to filter by
     * @return The iterator that will loop over the tree for the given patch.
     */
    template <class DV>
    PatchTreeIteratorBase<DV>* iterator(const Triple* triple_pattern) const;
    /**
     * Get the number of deletions for the given triple pattern.
     * @param triple_pattern The triple pattern to match by.
     * @param patch_id The patch id to get the deletions for, this patch id must exist within the tree!
     * @return A pair of the amount of deletions matching the given triple pattern for the given patch id and the last triple.
     *         This last triple will be undefined if the amount is zero.
     */
    std::pair<PatchPosition, Triple> deletion_count(const Triple &triple_pattern, int patch_id) const;
    /**
     * Get an iterator that loops over all deletions starting from a given triple and only matching the
     * given triple pattern.
     * @param offset The triple to start from.
     * @param patch_id The patch id to filter by, this includes all patches before this one. -1 Won't filter on patches.
     * @param triple_pattern Only triples that match the given pattern will be returned in the iterator.
     * @return The iterator that will loop over the tree for the given patch.
     */
    PositionedTripleIterator* deletion_iterator_from(const Triple& offset, int patch_id, const Triple& triple_pattern) const;
    /**
     * Get the deletion value for the given triple.
     * @param triple The triple to find
     * @return The deletion value for the given triple, or null.
     */
    PatchTreeDeletionValue* get_deletion_value(const Triple& triple) const;
    /**
     * Get the deletion value at or after the given triple pattern.
     * If the pattern is an exact match, then the exact value will be returned.
     * Otherwise the earliest value after that matching the triple pattern will be returned.
     * @param triple_pattern The triple pattern to lookup
     * @return The deletion value for the given triple pattern, or null.
     */
    template <class DV>
    PatchTreeDeletionValueBase<DV>* get_deletion_value_after(const Triple& triple_pattern) const;
    /**
     * Calculate the patch positions for the current triple for the current patch id.
     * @param triple The triple to calculate the patch positions for.
     * @param patch_id The patch in which to calculate the positions for this triple.
     * @return The patch positions for the given triple in the given patch id.
     */
    PatchPositions get_deletion_patch_positions(const Triple& triple, int patch_id, bool override___ = false, PatchPosition ___ = 0) const;
    /**
     * Get an iterator that loops over all additions starting with a given offset and only matching the
     * given triple pattern.
     * @param offset The number of addition triples to skip.
     * @param patch_id The patch id to filter by, this includes all patches before this one.
     * @param triple_pattern Only triples that match the given pattern will be returned in the iterator.
     * @return The iterator that will loop over the tree for the given patch.
     */
    PatchTreeTripleIterator* addition_iterator_from(long offset, int patch_id, const Triple& triple_pattern) const;
    /**
     * Get an iterator that loops over all additions matching given triple pattern.
     * @param triple_pattern Only triples that match the given pattern will be returned in the iterator.
     * @return The iterator that will loop over the tree for the additions.
     */
    PatchTreeIterator* addition_iterator(const Triple& triple_pattern) const;
    /**
     * Get the addition value for the given triple.
     * @param triple The triple to find
     * @return The addition value for the given triple, or null.
     */
    PatchTreeAdditionValue* get_addition_value(const Triple& triple) const;
    /**
     * Get the number of additions in the given patch id for the given triple pattern.
     * @param patch_id The patch id to filter by, this includes all patches before this one.
     * @param triple_pattern Only triples that match the given pattern will be returned in the iterator.
     * @return The iterator that will loop over the tree for the given patch.
     */
    PatchPosition addition_count(int patch_id, const Triple& triple_pattern) const;

    /**
     * @return The comparator for this patch tree in SPO order.
     */
    PatchTreeKeyComparator* get_spo_comparator() const;
    /**
     * @return The comparator for this patch tree in SPO order.
     */
    PatchElementComparator* get_element_comparator() const;
    /**
     * @return The largest patch id that is currently available.
     */
    int get_max_patch_id() const;
    /**
     * @return The smallest patch id that is currently available.
     */
    int get_min_patch_id() const;
protected:
    void write_metadata();
    void read_metadata();
};

#endif //TPFPATCH_STORE_PATCH_TREE_H
