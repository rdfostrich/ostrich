#include <iostream>
#include <kchashdb.h>

#include "../../main/cpp/patch/patch_tree.h"
#include "../../main/cpp/snapshot/snapshot_manager.h"
#include "../../main/cpp/snapshot/vector_triple_iterator.h"
#include "../../main/cpp/controller/controller.h"

#define BASEURI "<http://example.org>"

using namespace std;
using namespace kyotocabinet;

int main() {
  //Controller controller;

  // Build a snapshot
  /*std::vector<TripleString> triples;
  for(int i = 0; i < 10; i++) {
    string e = std::to_string(i);
    triples.push_back(TripleString(e, e, e));
  }
  VectorTripleIterator* it = new VectorTripleIterator(triples);
  controller.get_snapshot_manager()->create_snapshot(0, it, BASEURI);
  DictionaryManager* dict = controller.get_dictionary_manager(0);
  ModifiableDictionary* patchDict = dict->getPatchDict();

  Patch patch1(dict);
  patch1.add(PatchElement(Triple("0", "0", "0", patchDict), false));
  patch1.add(PatchElement(Triple("1", "1", "1", patchDict), false));
  patch1.add(PatchElement(Triple("2", "2", "2", patchDict), false));
  patch1.add(PatchElement(Triple("4", "4", "4", patchDict), false));
  patch1.add(PatchElement(Triple("5", "5", "5", patchDict), false));
  patch1.add(PatchElement(Triple("0", "6", "5", patchDict), false));
  patch1.add(PatchElement(Triple("0", "6", "6", patchDict), false));
  controller.append(patch1, 1, dict);

  // ----- TEST -----
  TripleIterator* ti = controller.get(Triple("", "", "", patchDict), 0, 1);
  Triple t;

  ti->next(&t);
  cout << t.to_string() << endl;

  ti->next(&t);
  cout << t.to_string() << endl;

  ti->next(&t);
  cout << t.to_string() << endl;


  // ----- CLEANUP -----
  // Delete patch files
  std::map<int, PatchTree*> patches = controller.get_patch_tree_manager()->get_patch_trees();
  std::map<int, PatchTree*>::iterator itP = patches.begin();
  while(itP != patches.end()) {
    int id = itP->first;
    std::remove(PATCHTREE_FILENAME(id, "spo").c_str());
    std::remove(PATCHTREE_FILENAME(id, "pos").c_str());
    std::remove(PATCHTREE_FILENAME(id, "pso").c_str());
    std::remove(PATCHTREE_FILENAME(id, "sop").c_str());
    std::remove(PATCHTREE_FILENAME(id, "osp").c_str());
    itP++;
  }

  // Delete snapshot files
  std::map<int, HDT*> snapshots = controller.get_snapshot_manager()->get_snapshots();
  std::map<int, HDT*>::iterator itS = snapshots.begin();
  while(itS != snapshots.end()) {
    int id = itS->first;
    std::remove(SNAPSHOT_FILENAME_BASE(id).c_str());
    std::remove((SNAPSHOT_FILENAME_BASE(id) + ".index").c_str());
    itS++;
  }*/

  // Create dictmanager
  /*DictionaryManager *dict;
  dict = new DictionaryManager(0);
  std::string a = "a";
  std::string b = "b";
  int ia = dict->insert(a, SUBJECT);
  int ib = dict->insert(b, SUBJECT);
  cout << "a:" << ia << endl;
  cout << "b:" << ib << endl;
  delete dict;

  DictionaryManager *dict2;
  dict2 = new DictionaryManager(0);
  int ib2 = dict2->stringToId(b, SUBJECT);
  int ia2 = dict2->stringToId(a, SUBJECT);
  cout << "a2:" << ia2 << endl;
  cout << "b2:" << ib2 << endl;
  delete dict2;

  // Delete dict file
  std::remove(PATCHDICT_FILENAME_BASE(0).c_str());*/

  /*DictionaryManager* dict = new DictionaryManager(0);
  Triple triple("a", "a", "a", dict);

  cout << triple.get_subject(*dict) << endl;
  cout << triple.get_predicate(*dict) << endl;
  cout << triple.get_object(*dict) << endl;

  delete dict;
  DictionaryManager::cleanup(0);*/


  /*SnapshotManager snapshotManager;
  std::map<int, HDT*> snapshots = snapshotManager.detect_snapshots();
  cout << snapshots.size() << endl;*/

  /*cout << snapshotManager.get_latest_snapshot(0) << endl;
  cout << snapshotManager.get_latest_snapshot(5) << endl;
  cout << snapshotManager.get_latest_snapshot(10) << endl;
  cout << snapshotManager.get_latest_snapshot(15) << endl;*/

  /*std::remove("true-patches.kct");
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
*/
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
