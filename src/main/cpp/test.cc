#include <iostream>
#include <kchashdb.h>

#include "patch/patch_tree.h"

using namespace std;
using namespace kyotocabinet;

int main() {
  std::remove("true-patches.kct");
  PatchTree patchTree("true-patches.kct");

  cout << "-----" << endl;
  Patch patch1;
  patch1.add(PatchElement(Triple("g", "p", "o"), false));
  patch1.add(PatchElement(Triple("a", "p", "o"), true));
  patchTree.append(patch1, 1);
  cout << "-----" << endl;
  Patch patch3;
  patch3.add(PatchElement(Triple("a", "p", "o"), false));
  patchTree.append(patch3, 2);
  cout << "-----" << endl;
  Patch patch4;
  patch4.add(PatchElement(Triple("a", "p", "o"), true));
  patchTree.append(patch4, 3);
  cout << "-----" << endl;
  Patch patch5;
  patch5.add(PatchElement(Triple("a", "p", "o"), false));
  patchTree.append(patch5, 4);
  cout << "-----" << endl;

  cout << "-" << patchTree.deletion_count(Triple("", "", ""), 1) << endl;
  cout << patchTree.reconstruct_patch(1, false).to_string() << endl;
  cout << "-" << patchTree.deletion_count(Triple("", "", ""), 2) << endl;
  cout << patchTree.reconstruct_patch(2, false).to_string() << endl;
  cout << "-" << patchTree.deletion_count(Triple("", "", ""), 3) << endl;
  cout << patchTree.reconstruct_patch(3, false).to_string() << endl;
  //cout << patchTree.reconstruct_patch(2).to_string() << endl;

  /*Patch p1;
  PatchElement pe = PatchElement(Triple("a", "p", "o"), false);
  //pe.set_local_change(true);
  p1.add(pe);
  p1.add(PatchElement(Triple("a", "p", "o"), true));
  cout << p1.to_string() << endl;

  Patch derived = p1.apply_local_changes();
  cout << derived.to_string() << endl;*/

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
