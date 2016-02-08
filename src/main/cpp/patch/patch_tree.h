#ifndef TPFPATCH_STORE_PATCH_TREE_H
#define TPFPATCH_STORE_PATCH_TREE_H

#include <string>
#include <kchashdb.h>

using namespace std;
using namespace kyotocabinet;

typedef struct Triple {
    string subject;
    string predicate;
    string object;
    Triple(string subject, string predicate, string object) :
            subject(subject), predicate(predicate), object(object) {}
} Triple; // TODO: use dictionary

typedef struct PatchElement {
    Triple triple;
    bool addition;
    PatchElement* next;
    PatchElement(Triple triple, bool addition) : triple(triple), addition(addition) {}
} PatchElement;

typedef Triple PatchTreeKey;

typedef struct PatchTreeValue {
    int patch_id;
    int patch_position;
    PatchTreeValue* next;
} PatchTreeValue;

class PatchTree
{
private:
    TreeDB db;
public:
    PatchTree(string file_name);
    ~PatchTree();
    void append(PatchElement* patch);
};

#endif //TPFPATCH_STORE_PATCH_TREE_H
