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

    PatchElements patchElements();
    patchElements.add(PatchElement(Triple("s1", "p1", "o1"), true));
    patchElements.add(PatchElement(Triple("s2", "p2", "o2"), false));
    patchElements.add(PatchElement(Triple("s3", "p3", "o3"), false));
    patchElements.add(PatchElement(Triple("s4", "p4", "o4"), true));

    /*PatchElements* patchElements = (PatchElements *) malloc(sizeof(PatchElements) * 4);
    PatchElements* fillPatchElements = patchElements;
    *fillPatchElements = PatchElements(patchElement1, true);
    fillPatchElements += sizeof(PatchElements);
    *fillPatchElements = PatchElements(patchElement2, true);
    fillPatchElements += sizeof(PatchElements);
    *fillPatchElements = PatchElements(patchElement3, true);
    fillPatchElements += sizeof(PatchElements);
    *fillPatchElements = PatchElements(patchElement4, false);*/

    PatchTree patchTree("true-patches.kch");
    int patch_id = 0;
    patchTree.append(patchElements, patch_id); // TODO: if you want to insert

    //PatchTreeKey key("s1", "p1", "o1");
    PatchTreeKey key("s2", "p2", "o2");
    PatchTreeIterator patchTreeIterator = patchTree.iterator(&key);
    PatchTreeKey* k;
    PatchTreeValue* v;
    cout << "BEGIN LOOP" << endl;
    while (patchTreeIterator.next(&k, &v)) {
        cout << ">> " << k->subject << " " << k->predicate << " " << k->object << "." << ": (" << v->addition << "; " << v->patch_id << ")" << endl;
    }
    cout << "END LOOP" << endl;

    return 0;
}

