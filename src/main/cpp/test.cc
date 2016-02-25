#include <iostream>
#include <kchashdb.h>

#include "patch/patch_tree.h"

using namespace std;
using namespace kyotocabinet;

int main() {
  std::remove("true-patches.kch");
  PatchTree patchTree("true-patches.kch");

  /*Patch patch;
  patch.add(PatchElement(Triple("s1", "p1", "o1"), false));
  patch.add(PatchElement(Triple("s2", "p2", "o2"), false));
  patch.add(PatchElement(Triple("s3", "p3", "o3"), false));
  patch.add(PatchElement(Triple("s4", "p4", "o4"), true));
  patchTree.append(patch, 0);

  Patch patch1;
  patch1.add(PatchElement(Triple("s1", "p1", "o1"), false));
  patchTree.append(patch1, 1);

  Patch patch2;
  patch2.add(PatchElement(Triple("s1", "p1", "o1"), true));
  patchTree.append(patch2, 2);*/
  cout << "-----" << endl;
  Patch patch1;
  patch1.add(PatchElement(Triple("g", "p", "o"), false));
  patch1.add(PatchElement(Triple("a", "p", "o"), true));
  patchTree.append(patch1, 1);
cout << "-----" << endl;
  Patch patch2;
  patch2.add(PatchElement(Triple("s", "z", "o"), false));
  patch2.add(PatchElement(Triple("s", "a", "o"), true));
  patchTree.append(patch2, 1);
  cout << "-----" << endl;
  Patch patch3;
  patch3.add(PatchElement(Triple("g", "p", "o"), false));
  patch3.add(PatchElement(Triple("a", "p", "o"), false));
  patch3.add(PatchElement(Triple("h", "z", "o"), false));
  patch3.add(PatchElement(Triple("l", "a", "o"), true));
  patchTree.append(patch3, 2);
  cout << "-----" << endl;
  Patch patch4;
  patch4.add(PatchElement(Triple("x", "y", "z"), false));
  patchTree.append(patch4, 3);
  cout << "-----" << endl;

  cout << patchTree.deletion_count(Triple("", "", ""), 1) << endl;
  cout << patchTree.reconstruct_patch(1).to_string() << endl;
  cout << patchTree.deletion_count(Triple("", "", ""), 2) << endl;
  cout << patchTree.reconstruct_patch(2).to_string() << endl;
  cout << patchTree.deletion_count(Triple("", "", ""), 3) << endl;
  cout << patchTree.reconstruct_patch(3).to_string() << endl;
  //cout << patchTree.reconstruct_patch(2).to_string() << endl;

  // PatchTreeKey key("s1", "p1", "o1");
  /*PatchTreeKey key("s", "p", "o");
  PatchTreeIterator patchTreeIterator = patchTree.iterator(&key);
  PatchTreeKey k;
  PatchTreeValue v;
  cout << "BEGIN LOOP" << endl;
  while (patchTreeIterator.next(&k, &v)) {
    cout << ">> " << k.to_string() << " :: " << v.to_string() << endl;
  }
  cout << "END LOOP" << endl;*/

  return 0;
}
