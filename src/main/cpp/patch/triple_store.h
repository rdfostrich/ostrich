#ifndef TPFPATCH_STORE_TRIPLE_STORE_H
#define TPFPATCH_STORE_TRIPLE_STORE_H

#include <iterator>
#include <kchashdb.h>
#include "triple.h"
#include "patch.h"
#include "../dictionary/dictionary_manager.h"
#include "patch_tree_key_comparator.h"
#include "patch_tree_addition_value.h"


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
// The minimum addition triple count so that it will be stored in the db
#ifndef MIN_ADDITION_COUNT
#define MIN_ADDITION_COUNT 100
#endif

class TripleStore {
private:
    kyotocabinet::TreeDB* index_spo_deletions;
    kyotocabinet::TreeDB* index_pos_deletions;
    kyotocabinet::TreeDB* index_osp_deletions;
    kyotocabinet::TreeDB* index_spo_additions;
    kyotocabinet::TreeDB* index_pos_additions;
    kyotocabinet::TreeDB* index_osp_additions;
    kyotocabinet::HashDB* count_additions;
    kyotocabinet::HashDB* temp_count_additions;
    //TreeDB index_ops; // We don't need this one if we maintain our s,p,o order priorites
    std::shared_ptr<DictionaryManager> dict;
    PatchTreeKeyComparator* spo_comparator;
    PatchTreeKeyComparator* pos_comparator;
    PatchTreeKeyComparator* osp_comparator;
    PatchElementComparator* element_comparator;
    int flush_counter_additions = 0;
    int flush_counter_deletions = 0;
protected:
    void open(kyotocabinet::TreeDB* db, string name, bool readonly);
    void close(kyotocabinet::TreeDB* db, string name);
    void increment_addition_count(const TripleVersion& triple_version);
public:
    TripleStore(string base_file_name, std::shared_ptr<DictionaryManager> dict, int8_t kc_opts = 0, bool readonly = false);
    ~TripleStore();
    kyotocabinet::TreeDB* getAdditionsTree(Triple triple_pattern);
    kyotocabinet::TreeDB* getDefaultAdditionsTree();
    kyotocabinet::TreeDB* getDeletionsTree(Triple triple_pattern);
    kyotocabinet::TreeDB* getDefaultDeletionsTree();
    void insertAdditionSingle(const PatchTreeKey* key, const PatchTreeAdditionValue* value, kyotocabinet::DB::Cursor* cursor = nullptr);
    void insertAdditionSingle(const PatchTreeKey* key, int patch_id, bool local_change, bool ignore_existing, kyotocabinet::DB::Cursor* cursor = nullptr);
    void increment_addition_counts(int patch_id, const Triple& triple);
    PatchPosition get_addition_count(int patch_id, const Triple& triple);
    long flush_addition_counts();
    void insertDeletionSingle(const PatchTreeKey* key, const PatchTreeDeletionValue* value, const PatchTreeDeletionValueReduced* value_reduced, kyotocabinet::DB::Cursor* cursor = nullptr);
    void insertDeletionSingle(const PatchTreeKey* key, const PatchPositions& patch_positions, int patch_id, bool local_change, bool ignore_existing, kyotocabinet::DB::Cursor* cursor = nullptr);
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
    std::shared_ptr<DictionaryManager> get_dict_manager() const;
    /**
     * @param triple_pattern A triple pattern
     * @return If the given triple pattern applies to a default addition or deletions tree, i.e., if it is using SPO order.
     */
    static inline bool is_default_tree(const Triple& triple_pattern) {
        bool s = triple_pattern.get_subject() > 0;
        bool p = triple_pattern.get_predicate() > 0;
        bool o = triple_pattern.get_object() > 0;
        return (s && p) || (!p && !o);
    }

    static inline hdt::TripleComponentOrder get_query_order(const Triple& triple_pattern) {
        bool s = triple_pattern.get_subject() > 0;
        bool p = triple_pattern.get_predicate() > 0;
        bool o = triple_pattern.get_object() > 0;

        if( s &  p &  o) return hdt::SPO;
        if( s &  p & !o) return hdt::SPO;
        if( s & !p &  o) return hdt::OSP;
        if( s & !p & !o) return hdt::SPO;
        if(!s &  p &  o) return hdt::POS;
        if(!s &  p & !o) return hdt::POS;
        if(!s & !p &  o) return hdt::OSP;
        return hdt::SPO;
    }

    static inline hdt::TripleComponentOrder get_query_order(const StringTriple& triple_pattern) {
        size_t s = triple_pattern.get_subject().empty() ? 0 : 1;
        size_t p = triple_pattern.get_predicate().empty() ? 0 : 1;
        size_t o = triple_pattern.get_object().empty() ? 0 : 1;

        return get_query_order(Triple(s, p, o));
    }

};

#endif //TPFPATCH_STORE_TRIPLE_STORE_H
