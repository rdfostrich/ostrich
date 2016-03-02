#include "triple_store.h"
#include "patch_tree_key_comparator.h"

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
    // TODO: other one for each index
    index_spo->tune_comparator(new PatchTreeKeyComparator());
    index_sop->tune_comparator(new PatchTreeKeyComparator());
    index_pso->tune_comparator(new PatchTreeKeyComparator());
    index_pos->tune_comparator(new PatchTreeKeyComparator());
    index_osp->tune_comparator(new PatchTreeKeyComparator());

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

TreeDB* TripleStore::getTree() {
    return index_spo;
}
