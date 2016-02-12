#include <iostream>
#include <kchashdb.h>

#include "patch/patch_tree.h"

using namespace std;
using namespace kyotocabinet;

int main() {
    Patch patch;
    patch.add(PatchElement(Triple("s1", "p1", "o1"), true));
    patch.add(PatchElement(Triple("s2", "p2", "o2"), false));
    patch.add(PatchElement(Triple("s3", "p3", "o3"), false));
    patch.add(PatchElement(Triple("s4", "p4", "o4"), true));

    PatchTree patchTree("true-patches.kch");
    int patch_id = 0;
    patchTree.append_unsafe(patch, patch_id); // TODO: if you want to insert

    //PatchTreeKey key("s1", "p1", "o1");
    PatchTreeKey key("s", "p", "o");
    PatchTreeIterator patchTreeIterator = patchTree.iterator(&key);
    PatchTreeKey k;
    PatchTreeValue v;
    cout << "BEGIN LOOP" << endl;
    while (patchTreeIterator.next(&k, &v)) {
        cout << ">> " << k.to_string() << " :: " << v.to_string() << endl;
    }
    cout << "END LOOP" << endl;

    return 0;
}

