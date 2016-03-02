#ifndef TPFPATCH_STORE_TRIPLE_STORE_H
#define TPFPATCH_STORE_TRIPLE_STORE_H

#include <iterator>
#include <kchashdb.h>
#include "triple.h"
#include "patch.h"

using namespace std;
using namespace kyotocabinet;

class TripleStore {
private:
    TreeDB* index_spo;
    TreeDB* index_sop;
    TreeDB* index_pso;
    TreeDB* index_pos;
    TreeDB* index_osp;
    //TreeDB index_ops; // We don't need this one if we maintain our s,p,o order priorites
protected:
    void open(TreeDB* db, string name);
    void close(TreeDB* db, string name);
public:
    TripleStore(string base_file_name);
    ~TripleStore();
    TreeDB* getTree(Triple triple_pattern);
    bool isDefaultTree(Triple triple_pattern);
    TreeDB* getTree();
    void insertAddition(Patch* patch, int patch_id);
};


#endif //TPFPATCH_STORE_TRIPLE_STORE_H
