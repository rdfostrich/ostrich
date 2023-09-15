#include "triple_store.h"
#include "patch_tree_addition_value.h"
#include "patch_tree_key_comparator.h"
#include "../simpleprogresslistener.h"


TripleStore::TripleStore(string base_file_name, std::shared_ptr<DictionaryManager> dict, int8_t kc_opts, bool readonly) : dict(dict) {
    // Construct trees
    index_spo_deletions = new kyotocabinet::TreeDB();
    index_pos_deletions = new kyotocabinet::TreeDB();
    index_osp_deletions = new kyotocabinet::TreeDB();
    index_spo_additions = new kyotocabinet::TreeDB();
    index_pos_additions = new kyotocabinet::TreeDB();
    index_osp_additions = new kyotocabinet::TreeDB();
    count_additions = new kyotocabinet::HashDB();
    temp_count_additions = readonly ? nullptr : new kyotocabinet::HashDB();

    // Set the triple comparators
    index_spo_deletions->tune_comparator(spo_comparator = new PatchTreeKeyComparator(comp_s, comp_p, comp_o, dict));
    index_pos_deletions->tune_comparator(pos_comparator = new PatchTreeKeyComparator(comp_p, comp_o, comp_s, dict));
    index_osp_deletions->tune_comparator(osp_comparator = new PatchTreeKeyComparator(comp_o, comp_s, comp_p, dict));
    index_spo_additions->tune_comparator(spo_comparator);
    index_pos_additions->tune_comparator(pos_comparator);
    index_osp_additions->tune_comparator(osp_comparator);
    element_comparator = new PatchElementComparator(spo_comparator);

    index_spo_deletions->tune_options(kc_opts);
    index_pos_deletions->tune_options(kc_opts);
    index_osp_deletions->tune_options(kc_opts);
    index_spo_additions->tune_options(kc_opts);
    index_pos_additions->tune_options(kc_opts);
    index_osp_additions->tune_options(kc_opts);

    // Open the databases
    open(index_spo_deletions, base_file_name + "_spo_deletions", readonly);
    open(index_pos_deletions, base_file_name + "_pos_deletions", readonly);
    open(index_osp_deletions, base_file_name + "_osp_deletions", readonly);
    open(index_spo_additions, base_file_name + "_spo_additions", readonly);
    open(index_pos_additions, base_file_name + "_pos_additions", readonly);
    open(index_osp_additions, base_file_name + "_osp_additions", readonly);
    if (!count_additions->open(base_file_name + "_count_additions", (readonly ? kyotocabinet::HashDB::OREADER : (kyotocabinet::HashDB::OWRITER | kyotocabinet::HashDB::OCREATE)) | kyotocabinet::HashDB::ONOREPAIR)) {
        cerr << "Open addition count tree error: " << count_additions->error().name() << endl;
    }
    if (temp_count_additions != nullptr) {
        if (!temp_count_additions->open(base_file_name + "_count_additions.tmp",
                                        (readonly ? kyotocabinet::HashDB::OREADER : (kyotocabinet::HashDB::OWRITER | kyotocabinet::HashDB::OCREATE)) |
                                                kyotocabinet::HashDB::ONOREPAIR)) {
            cerr << "Open addition count tree error: " << temp_count_additions->error().name() << endl;
        }
    }
}

TripleStore::~TripleStore() {
    // Close the databases
    close(index_spo_deletions, "spo_deletions");
    close(index_pos_deletions, "pos_deletions");
    close(index_osp_deletions, "osp_deletions");
    close(index_spo_additions, "spo_additions");
    close(index_pos_additions, "pos_additions");
    close(index_osp_additions, "osp_additions");

    if (!count_additions->close()) {
        cerr << "Close addition count tree error: " << count_additions->error().name() << endl;
    }
    delete count_additions;
    if (temp_count_additions != nullptr) {
        string path = temp_count_additions->path();
        if (!temp_count_additions->close()) {
            cerr << "Close temp addition count tree error: " << temp_count_additions->error().name() << endl;
        }
        delete temp_count_additions;
        std::remove(path.c_str());
    }

    delete spo_comparator;
    delete pos_comparator;
    delete osp_comparator;
    delete element_comparator;
}

void TripleStore::open(kyotocabinet::TreeDB* db, string name, bool readonly) {
    db->tune_map(KC_MEMORY_MAP_SIZE);
    //db->tune_buckets(1LL * 1000 * 1000);
    db->tune_page_cache(KC_PAGE_CACHE_SIZE);
    db->tune_defrag(8);
    if (!db->open(name, (readonly ? kyotocabinet::TreeDB::OREADER : (kyotocabinet::TreeDB::OWRITER | kyotocabinet::TreeDB::OCREATE)) | kyotocabinet::TreeDB::ONOREPAIR)) {
        cerr << "open " << name << " error: " << db->error().name() << endl;
    }
}

void TripleStore::close(kyotocabinet::TreeDB* db, string name) {
    if (!db->close()) {
        cerr << "close " << name << " error: " << db->error().name() << endl;
    }
    delete db;
}

kyotocabinet::TreeDB* TripleStore::getAdditionsTree(Triple triple_pattern) {
    hdt::TripleComponentOrder order = get_query_order(triple_pattern);

    if(order == hdt::OSP) return index_osp_additions;
    if(order == hdt::POS) return index_pos_additions;
    return index_spo_additions;
}

kyotocabinet::TreeDB* TripleStore::getDefaultAdditionsTree() {
    return index_spo_additions;
}

kyotocabinet::TreeDB* TripleStore::getDeletionsTree(Triple triple_pattern) {
    hdt::TripleComponentOrder order = get_query_order(triple_pattern);

    if(order == hdt::OSP) return index_osp_deletions;
    if(order == hdt::POS) return index_pos_deletions;
    return index_spo_deletions;
}

kyotocabinet::TreeDB* TripleStore::getDefaultDeletionsTree() {
    return index_spo_deletions;
}

void TripleStore::insertAdditionSingle(const PatchTreeKey* key, const PatchTreeAdditionValue* value, kyotocabinet::DB::Cursor* cursor) {
    size_t key_size, value_size;
    const char *raw_key = key->serialize(&key_size);
    const char *raw_value = value->serialize(&value_size);

    if (cursor != nullptr) {
        cursor->set_value(raw_value, value_size, false);
    } else {
        index_spo_additions->set(raw_key, key_size, raw_value, value_size);
    }
    index_pos_additions->set(raw_key, key_size, raw_value, value_size);
    index_osp_additions->set(raw_key, key_size, raw_value, value_size);

    delete[] raw_key;
    delete[] raw_value;

    // Flush db to disk
    if (++flush_counter_additions > FLUSH_TRIPLES_COUNT) {
        index_spo_additions->synchronize();
        index_pos_additions->synchronize();
        index_osp_additions->synchronize();
        flush_counter_additions = 0;
    }
}

void TripleStore::insertAdditionSingle(const PatchTreeKey* key, int patch_id, bool local_change, bool ignore_existing, kyotocabinet::DB::Cursor* cursor) {
    // Look up the value for the given triple key in the tree.
#ifdef COMPRESSED_ADD_VALUES
    PatchTreeAdditionValue value(patch_id);
#else
    PatchTreeAdditionValue value;
#endif
    if (!ignore_existing) {
        // We assume that are indexes are sane, we only check one of them
        size_t key_size, value_size;
        const char *raw_key = key->serialize(&key_size);
        const char *raw_value = cursor == nullptr ? index_spo_additions->get(raw_key, key_size, &value_size) : cursor->get_value(&value_size, false);
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
    if (!local_change) increment_addition_counts(patch_id, *key);
}

void TripleStore::insertDeletionSingle(const PatchTreeKey* key, const PatchTreeDeletionValue* value, const PatchTreeDeletionValueReduced* value_reduced, kyotocabinet::DB::Cursor* cursor) {
    size_t key_size, value_size, value_reduced_size;
    const char *raw_key = key->serialize(&key_size);
    const char *raw_value = value->serialize(&value_size);
    const char *raw_value_reduced = value_reduced->serialize(&value_reduced_size);

    if (cursor != nullptr) {
        cursor->set_value(raw_value, value_size, false);
    } else {
        index_spo_deletions->set(raw_key, key_size, raw_value, value_size);
    }
    index_pos_deletions->set(raw_key, key_size, raw_value_reduced, value_reduced_size);
    index_osp_deletions->set(raw_key, key_size, raw_value_reduced, value_reduced_size);

    delete[] raw_key;
    delete[] raw_value;
    delete[] raw_value_reduced;

    // Flush db to disk
    if (++flush_counter_deletions > FLUSH_TRIPLES_COUNT) {
        index_spo_deletions->synchronize();
        index_pos_deletions->synchronize();
        index_osp_deletions->synchronize();
        flush_counter_deletions = 0;
    }
}

void TripleStore::insertDeletionSingle(const PatchTreeKey* key, const PatchPositions& patch_positions, int patch_id, bool local_change, bool ignore_existing, kyotocabinet::DB::Cursor* cursor) {
#ifdef COMPRESSED_DEL_VALUES
    PatchTreeDeletionValue deletion_value(patch_id);
#else
    PatchTreeDeletionValue deletion_value;
#endif
    if (!ignore_existing) {
        size_t key_size, value_size;
        const char *raw_key = key->serialize(&key_size);
        const char *raw_value = cursor == nullptr ? index_spo_deletions->get(raw_key, key_size, &value_size) : cursor->get_value(&value_size, false);
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

    // Convert our full deletion value to a reduced one
    PatchTreeDeletionValueReduced deletion_value_reduced = deletion_value.to_reduced();
    insertDeletionSingle(key, &deletion_value, &deletion_value_reduced);
}

PatchTreeKeyComparator *TripleStore::get_spo_comparator() const {
    return spo_comparator;
}

PatchElementComparator *TripleStore::get_element_comparator() const {
    return element_comparator;
}

std::shared_ptr<DictionaryManager> TripleStore::get_dict_manager() const {
    return dict;
}

void TripleStore::increment_addition_counts(const int patch_id, const Triple &triple) {
    increment_addition_count(TripleVersion(patch_id, Triple(triple.get_subject(), triple.get_predicate(), 0                  )));
    increment_addition_count(TripleVersion(patch_id, Triple(triple.get_subject(), 0                     , 0                  )));
    increment_addition_count(TripleVersion(patch_id, Triple(triple.get_subject(), 0                     , triple.get_object())));
    increment_addition_count(TripleVersion(patch_id, Triple(0                   , triple.get_predicate(), 0                  )));
    increment_addition_count(TripleVersion(patch_id, Triple(0                   , triple.get_predicate(), triple.get_object())));
    increment_addition_count(TripleVersion(patch_id, Triple(0                   , 0                     , 0                  )));
    increment_addition_count(TripleVersion(patch_id, Triple(0                   , 0                     , triple.get_object())));
}

void TripleStore::increment_addition_count(const TripleVersion& triple_version) {
    size_t _, tv_size;
    char* raw_value;
    const char* raw_key = triple_version.serialize(&tv_size);
    raw_value = temp_count_additions->get(raw_key, tv_size, &_);
    PatchPosition pos = 0;
    if (raw_value != nullptr) {
        std::memcpy(&pos, raw_value, sizeof(PatchPosition));
    } else {
        raw_value = new char[sizeof(PatchPosition)];
    }
    pos++;
    std::memcpy(raw_value, &pos, sizeof(PatchPosition));
    temp_count_additions->set(raw_key, tv_size, raw_value, sizeof(PatchPosition));

    delete[] raw_key;
    delete[] raw_value;
}

PatchPosition TripleStore::get_addition_count(const int patch_id, const Triple &triple) {
    size_t ksp, vsp;
    TripleVersion key(patch_id, triple);
    const char* kbp = key.serialize(&ksp);
    const char* vbp = count_additions->get(kbp, ksp, &vsp);
    PatchPosition count = 0;
    if (vbp != nullptr) {
        std::memcpy(&count, vbp, sizeof(PatchPosition));
    }
    delete[] kbp;
    delete[] vbp;
    return count;
}

long TripleStore::flush_addition_counts() {
    size_t ksp, vsp;
    PatchPosition count = 0;
    kyotocabinet::HashDB::Cursor* cursor = temp_count_additions->cursor();
    cursor->jump();
    long added = 0;
    while (cursor->step()) {
        const char* vbp;
        const char* kbp = cursor->get(&ksp, &vbp, &vsp, false);
        std::memcpy(&count, vbp, sizeof(PatchPosition));
        if (count >= MIN_ADDITION_COUNT) {
            count_additions->set(kbp, ksp, vbp, vsp);
            added++;
        }
        delete[] kbp;
    }
    delete cursor;
    count_additions->synchronize();
    temp_count_additions->clear();

    return added;
}
