#include <iostream>
#include <kchashdb.h>

#include "patch/patch_tree.h"

using namespace std;
using namespace kyotocabinet;

int main() {
    PatchElement patchElement1(Triple("s1", "p1", "o1"), true);
    PatchElement patchElement2(Triple("s2", "p2", "o2"), false);
    PatchElement patchElement3(Triple("s3", "p3", "o3"), false);
    PatchElement patchElement4(Triple("s4", "p4", "o4"), true);

    PatchElements patchElements1(patchElement1);
    PatchElements patchElements2(patchElement2);
    PatchElements patchElements3(patchElement3);
    PatchElements patchElements4(patchElement4);

    patchElements1.next = &patchElements2;
    patchElements2.next = &patchElements3;
    patchElements3.next = &patchElements4;
    patchElements4.next = NULL;

    PatchTree patchTree("true-patches.kch");
    //patchTree.append(&patchElements1); // TODO: if you want to insert

    //PatchTreeKey key("s1", "p1", "o1");
    PatchTreeKey key("s2", "p2", "o2");
    PatchTreeIterator patchTreeIterator = patchTree.iterator(&key);
    PatchTreeKey* k;
    bool* v;//PatchTreeValue* v; TODO
    cout << "BEGIN LOOP" << endl;
    while (patchTreeIterator.next(&k, &v)) {
        cout << ">> " << k->subject << " " << k->predicate << " " << k->object << "." << ":" << *v << endl;
    }
    cout << "END LOOP" << endl;

    return 0;
}

