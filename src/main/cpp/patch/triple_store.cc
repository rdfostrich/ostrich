#include "triple_store.h"
#include "patch_tree_key_comparator.h"
#include "patch_tree_addition_value.h"

using namespace std;
using namespace kyotocabinet;

TripleStore::TripleStore(string base_file_name) {
    // Construct trees
    index_spo = new TreeDB();
    index_sop = new TreeDB();
    index_pso = new TreeDB();
    index_pos = new TreeDB();
    index_osp = new TreeDB();

    // Set the triple comparators
    index_spo->tune_comparator(new PatchTreeKeyComparator(comp_s, comp_p, comp_o));
    index_sop->tune_comparator(new PatchTreeKeyComparator(comp_s, comp_o, comp_p));
    index_pso->tune_comparator(new PatchTreeKeyComparator(comp_p, comp_s, comp_o));
    index_pos->tune_comparator(new PatchTreeKeyComparator(comp_p, comp_o, comp_s));
    index_osp->tune_comparator(new PatchTreeKeyComparator(comp_o, comp_s, comp_p));

    // Open the databases
    open(index_spo, base_file_name + "_spo");
    open(index_sop, base_file_name + "_sop");
    open(index_pso, base_file_name + "_pso");
    open(index_pos, base_file_name + "_pos");
    open(index_osp, base_file_name + "_osp");
}

TripleStore::~TripleStore() {
    // Close the databases
    close(index_spo, "spo");
    close(index_sop, "sop");
    close(index_pso, "pso");
    close(index_pos, "pos");
    close(index_osp, "osp");
}

void TripleStore::open(TreeDB* db, string name) {
    if (!db->open(name, HashDB::OWRITER | HashDB::OCREATE)) {
        cerr << "open " << name << " error: " << db->error().name() << endl;
    }
}

void TripleStore::close(TreeDB* db, string name) {
    if (!db->close()) {
        cerr << "close " << name << " error: " << db->error().name() << endl;
    }
}

TreeDB* TripleStore::getTree(Triple triple_pattern) {
    bool s = triple_pattern.get_subject() != "";
    bool p = triple_pattern.get_predicate() != "";
    bool o = triple_pattern.get_object() != "";

    if( s &  p &  o) return index_spo;
    if( s &  p & !o) return index_spo;
    if( s & !p &  o) return index_sop;
    if( s & !p & !o) return index_spo;
    if(!s &  p &  o) return index_pos;
    if(!s &  p & !o) return index_pso;
    if(!s & !p &  o) return index_osp;
    /*if(!s & !p & !o) */return index_spo;
}

bool TripleStore::isDefaultTree(Triple triple_pattern) {
    bool s = triple_pattern.get_subject() != "";
    bool p = triple_pattern.get_predicate() != "";
    bool o = triple_pattern.get_object() != "";

    if( s &  p &  o) return true;
    if( s &  p & !o) return true;
    if(!s & !p & !o) return true;
    return false;
}

TreeDB* TripleStore::getTree() {
    return index_spo;
}

void TripleStore::insertAddition(Patch* patch, int patch_id) {
    for(int i = 0; i < patch->get_size(); i++) {
        PatchElement patchElement = patch->get(i);
        if(patchElement.is_addition() && !patchElement.is_local_change()) {
            // Look up the value for the given triple key in the tree.
            size_t key_size, value_size;
            const char *raw_key = patchElement.get_triple().serialize(&key_size);
            PatchTreeAdditionValue value;
            // We assume that are indexes are sane, we only check one of them
            const char *raw_value = index_sop->get(raw_key, key_size, &value_size);
            if (raw_value) {
                value.deserialize(raw_value, value_size);
            }
            value.add(patch_id);

            // Serialize the new value and store it
            size_t new_value_size;
            const char *new_raw_value = value.serialize(&new_value_size);

            // Don't insert into SPO, because that one has another type of values!
            index_sop->set(raw_key, key_size, new_raw_value, new_value_size);
            index_pso->set(raw_key, key_size, new_raw_value, new_value_size);
            index_pos->set(raw_key, key_size, new_raw_value, new_value_size);
            index_osp->set(raw_key, key_size, new_raw_value, new_value_size);
        }
    }
}
