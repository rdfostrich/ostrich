#include "triple_store.h"
#include "patch_tree_addition_value.h"
#include "patch_tree_key_comparator.h"
#include "../simpleprogresslistener.h"

using namespace std;
using namespace kyotocabinet;

TripleStore::TripleStore(string base_file_name, DictionaryManager* dict, int8_t kc_opts, bool readonly) : dict(dict) {
    // Construct trees
    index_spo_deletions = new TreeDB();
    index_sop_deletions = new TreeDB();
    index_pso_deletions = new TreeDB();
    index_pos_deletions = new TreeDB();
    index_osp_deletions = new TreeDB();
    index_spo_additions = new TreeDB();
    index_sop_additions = new TreeDB();
    index_pso_additions = new TreeDB();
    index_pos_additions = new TreeDB();
    index_osp_additions = new TreeDB();

    // Set the triple comparators
    index_spo_deletions->tune_comparator(spo_comparator = new PatchTreeKeyComparator(comp_s, comp_p, comp_o, dict));
    index_sop_deletions->tune_comparator(sop_comparator = new PatchTreeKeyComparator(comp_s, comp_o, comp_p, dict));
    index_pso_deletions->tune_comparator(pso_comparator = new PatchTreeKeyComparator(comp_p, comp_s, comp_o, dict));
    index_pos_deletions->tune_comparator(pos_comparator = new PatchTreeKeyComparator(comp_p, comp_o, comp_s, dict));
    index_osp_deletions->tune_comparator(osp_comparator = new PatchTreeKeyComparator(comp_o, comp_s, comp_p, dict));
    index_spo_additions->tune_comparator(spo_comparator);
    index_sop_additions->tune_comparator(sop_comparator);
    index_pso_additions->tune_comparator(pso_comparator);
    index_pos_additions->tune_comparator(pos_comparator);
    index_osp_additions->tune_comparator(osp_comparator);
    element_comparator = new PatchElementComparator(spo_comparator);

    index_spo_deletions->tune_options(kc_opts);
    index_sop_deletions->tune_options(kc_opts);
    index_pso_deletions->tune_options(kc_opts);
    index_pos_deletions->tune_options(kc_opts);
    index_osp_deletions->tune_options(kc_opts);
    index_spo_additions->tune_options(kc_opts);
    index_sop_additions->tune_options(kc_opts);
    index_pso_additions->tune_options(kc_opts);
    index_pos_additions->tune_options(kc_opts);
    index_osp_additions->tune_options(kc_opts);

    // Open the databases
    open(index_spo_deletions, base_file_name + "_spo_deletions", readonly);
    open(index_sop_deletions, base_file_name + "_sop_deletions", readonly);
    open(index_pso_deletions, base_file_name + "_pso_deletions", readonly);
    open(index_pos_deletions, base_file_name + "_pos_deletions", readonly);
    open(index_osp_deletions, base_file_name + "_osp_deletions", readonly);
    open(index_spo_additions, base_file_name + "_spo_additions", readonly);
    open(index_sop_additions, base_file_name + "_sop_additions", readonly);
    open(index_pso_additions, base_file_name + "_pso_additions", readonly);
    open(index_pos_additions, base_file_name + "_pos_additions", readonly);
    open(index_osp_additions, base_file_name + "_osp_additions", readonly);
}

TripleStore::~TripleStore() {
    // Close the databases
    close(index_spo_deletions, "spo_deletions");
    close(index_sop_deletions, "sop_deletions");
    close(index_pso_deletions, "pso_deletions");
    close(index_pos_deletions, "pos_deletions");
    close(index_osp_deletions, "osp_deletions");
    close(index_spo_additions, "spo_additions");
    close(index_sop_additions, "sop_additions");
    close(index_pso_additions, "pso_additions");
    close(index_pos_additions, "pos_additions");
    close(index_osp_additions, "osp_additions");

    delete spo_comparator;
    delete sop_comparator;
    delete pso_comparator;
    delete pos_comparator;
    delete osp_comparator;
    delete element_comparator;
}

void TripleStore::open(TreeDB* db, string name, bool readonly) {
    db->tune_map(KC_MEMORY_MAP_SIZE);
    //db->tune_buckets(1LL * 1000 * 1000);
    db->tune_page_cache(KC_PAGE_CACHE_SIZE);
    db->tune_defrag(8);
    if (!db->open(name, (readonly ? TreeDB::OREADER : (TreeDB::OWRITER | TreeDB::OCREATE)) | TreeDB::ONOREPAIR)) {
        cerr << "open " << name << " error: " << db->error().name() << endl;
    }
}

void TripleStore::close(TreeDB* db, string name) {
    if (!db->close()) {
        cerr << "close " << name << " error: " << db->error().name() << endl;
    }
    delete db;
}

TreeDB* TripleStore::getAdditionsTree(Triple triple_pattern) {
    bool s = triple_pattern.get_subject() > 0;
    bool p = triple_pattern.get_predicate() > 0;
    bool o = triple_pattern.get_object() > 0;

    if( s &  p &  o) return index_spo_additions;
    if( s &  p & !o) return index_spo_additions;
    if( s & !p &  o) return index_sop_additions;
    if( s & !p & !o) return index_spo_additions;
    if(!s &  p &  o) return index_pos_additions;
    if(!s &  p & !o) return index_pso_additions;
    if(!s & !p &  o) return index_osp_additions;
    /*if(!s & !p & !o) */return index_spo_additions;
}

TreeDB* TripleStore::getDefaultAdditionsTree() {
    return index_spo_additions;
}

TreeDB* TripleStore::getDeletionsTree(Triple triple_pattern) {
    bool s = triple_pattern.get_subject() > 0;
    bool p = triple_pattern.get_predicate() > 0;
    bool o = triple_pattern.get_object() > 0;

    if( s &  p &  o) return index_spo_deletions;
    if( s &  p & !o) return index_spo_deletions;
    if( s & !p &  o) return index_sop_deletions;
    if( s & !p & !o) return index_spo_deletions;
    if(!s &  p &  o) return index_pos_deletions;
    if(!s &  p & !o) return index_pso_deletions;
    if(!s & !p &  o) return index_osp_deletions;
    /*if(!s & !p & !o) */return index_spo_deletions;
}

TreeDB* TripleStore::getDefaultDeletionsTree() {
    return index_spo_deletions;
}

void TripleStore::insertAdditionSingle(const PatchTreeKey* key, const PatchTreeAdditionValue* value, DB::Cursor* cursor) {
    size_t key_size, value_size;
    const char *raw_key = key->serialize(&key_size);
    const char *raw_value = value->serialize(&value_size);

    if (cursor != NULL) {
        cursor->set_value(raw_value, value_size, false);
    } else {
        index_spo_additions->set(raw_key, key_size, raw_value, value_size);
    }
    index_sop_additions->set(raw_key, key_size, raw_value, value_size);
    index_pso_additions->set(raw_key, key_size, raw_value, value_size);
    index_pos_additions->set(raw_key, key_size, raw_value, value_size);
    index_osp_additions->set(raw_key, key_size, raw_value, value_size);

    free((char*) raw_key);
    free((char*) raw_value);

    // Flush db to disk
    if (++flush_counter_additions > FLUSH_TRIPLES_COUNT) {
        index_spo_additions->synchronize();
        index_sop_additions->synchronize();
        index_pso_additions->synchronize();
        index_pos_additions->synchronize();
        index_osp_additions->synchronize();
        flush_counter_additions = 0;
    }
}

void TripleStore::insertAdditionSingle(const PatchTreeKey* key, int patch_id, bool local_change, bool ignore_existing, DB::Cursor* cursor) {
    // Look up the value for the given triple key in the tree.
    PatchTreeAdditionValue value;
    if (!ignore_existing) {
        // We assume that are indexes are sane, we only check one of them
        size_t key_size, value_size;
        const char *raw_key = key->serialize(&key_size);
        const char *raw_value = cursor == NULL ? index_spo_additions->get(raw_key, key_size, &value_size) : cursor->get_value(&value_size, false);
        if (raw_value) {
            value.deserialize(raw_value, value_size);
            delete[] raw_value;
        }
        free((char*) raw_key);
    }
    value.add(patch_id);
    if (local_change) {
        value.set_local_change(patch_id);
    }

    insertAdditionSingle(key, &value);
}

void TripleStore::insertDeletionSingle(const PatchTreeKey* key, const PatchTreeDeletionValue* value, DB::Cursor* cursor) {
    size_t key_size, value_size;
    const char *raw_key = key->serialize(&key_size);
    const char *raw_value = value->serialize(&value_size);

    if (cursor != NULL) {
        cursor->set_value(raw_value, value_size, false);
    } else {
        index_spo_deletions->set(raw_key, key_size, raw_value, value_size);
    }
    index_sop_deletions->set(raw_key, key_size, raw_value, value_size);
    index_pso_deletions->set(raw_key, key_size, raw_value, value_size);
    index_pos_deletions->set(raw_key, key_size, raw_value, value_size);
    index_osp_deletions->set(raw_key, key_size, raw_value, value_size);

    free((char*) raw_key);
    free((char*) raw_value);

    // Flush db to disk
    if (++flush_counter_deletions > FLUSH_TRIPLES_COUNT) {
        index_spo_deletions->synchronize();
        index_sop_deletions->synchronize();
        index_pso_deletions->synchronize();
        index_pos_deletions->synchronize();
        index_osp_deletions->synchronize();
        flush_counter_deletions = 0;
    }
}

void TripleStore::insertDeletionSingle(const PatchTreeKey* key, const PatchPositions& patch_positions, int patch_id, bool local_change, bool ignore_existing, DB::Cursor* cursor) {
    PatchTreeDeletionValue deletion_value;
    if (!ignore_existing) {
        size_t key_size, value_size;
        const char *raw_key = key->serialize(&key_size);
        const char *raw_value = cursor == NULL ? index_spo_deletions->get(raw_key, key_size, &value_size) : cursor->get_value(&value_size, false);
        if (raw_value) {
            deletion_value.deserialize(raw_value, value_size);
            delete[] raw_value;
        }
        free((char*) raw_key);
    }
    PatchTreeDeletionValueElement element = PatchTreeDeletionValueElement(patch_id, patch_positions);
    if (local_change) {
        element.set_local_change();
    }
    deletion_value.add(element);

    insertDeletionSingle(key, &deletion_value);
}

PatchTreeKeyComparator *TripleStore::get_spo_comparator() const {
    return spo_comparator;
}

PatchElementComparator *TripleStore::get_element_comparator() const {
    return element_comparator;
}

DictionaryManager *TripleStore::get_dict_manager() const {
    return dict;
}
