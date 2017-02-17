#ifndef TPFPATCH_STORE_TRIPLE_STORE_H
#define TPFPATCH_STORE_TRIPLE_STORE_H

#include <iterator>
#include <kchashdb.h>
#include "triple.h"
#include "patch.h"
#include "../dictionary/dictionary_manager.h"
#include "patch_tree_key_comparator.h"
#include "patch_tree_addition_value.h"

using namespace std;
using namespace kyotocabinet;

// The amount of triples after which the store should be flushed to disk, to avoid memory issues
#ifndef FLUSH_TRIPLES_COUNT
#define FLUSH_TRIPLES_COUNT 500000
#endif
// The KC memory map size per tree (128MB)
#ifndef KC_MEMORY_MAP_SIZE
#define KC_MEMORY_MAP_SIZE (1LL << 27)
#endif
// The KC page cache size per tree (32MB)
#ifndef KC_PAGE_CACHE_SIZE
#define KC_PAGE_CACHE_SIZE (1LL << 25)
#endif

class TripleStore {
private:
    TreeDB* index_spo_deletions;
    TreeDB* index_sop_deletions;
    TreeDB* index_pso_deletions;
    TreeDB* index_pos_deletions;
    TreeDB* index_osp_deletions;
    TreeDB* index_spo_additions;
    TreeDB* index_sop_additions;
    TreeDB* index_pso_additions;
    TreeDB* index_pos_additions;
    TreeDB* index_osp_additions;
    //TreeDB index_ops; // We don't need this one if we maintain our s,p,o order priorites
    DictionaryManager* dict;
    PatchTreeKeyComparator* spo_comparator;
    PatchTreeKeyComparator* sop_comparator;
    PatchTreeKeyComparator* pso_comparator;
    PatchTreeKeyComparator* pos_comparator;
    PatchTreeKeyComparator* osp_comparator;
    PatchElementComparator* element_comparator;
    int flush_counter_additions = 0;
    int flush_counter_deletions = 0;
protected:
    void open(TreeDB* db, string name, bool readonly);
    void close(TreeDB* db, string name);
public:
    TripleStore(string base_file_name, DictionaryManager* dict, int8_t kc_opts = 0, bool readonly = false);
    ~TripleStore();
    TreeDB* getAdditionsTree(Triple triple_pattern);
    TreeDB* getDefaultAdditionsTree();
    TreeDB* getDeletionsTree(Triple triple_pattern);
    TreeDB* getDefaultDeletionsTree();
    void insertAdditionSingle(const PatchTreeKey* key, const PatchTreeAdditionValue* value, DB::Cursor* cursor = NULL);
    void insertAdditionSingle(const PatchTreeKey* key, int patch_id, bool local_change, bool ignore_existing, DB::Cursor* cursor = NULL);
    void insertDeletionSingle(const PatchTreeKey* key, const PatchTreeDeletionValue* value, DB::Cursor* cursor = NULL);
    void insertDeletionSingle(const PatchTreeKey* key, const PatchPositions& patch_positions, int patch_id, bool local_change, bool ignore_existing, DB::Cursor* cursor = NULL);
    /**
     * @return The comparator for this patch tree in SPO order.
     */
    PatchTreeKeyComparator* get_spo_comparator() const;
    /**
     * @return The comparator for this patch tree in SPO order.
     */
    PatchElementComparator* get_element_comparator() const;
    /**
     * @return The dictionary manager.
     */
    DictionaryManager* get_dict_manager() const;
};


#endif //TPFPATCH_STORE_TRIPLE_STORE_H
