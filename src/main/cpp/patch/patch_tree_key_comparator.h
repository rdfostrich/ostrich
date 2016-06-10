#ifndef TPFPATCH_STORE_PATCH_TREE_KEY_COMPARATOR_H
#define TPFPATCH_STORE_PATCH_TREE_KEY_COMPARATOR_H

#include <kchashdb.h>
#include <Dictionary.hpp>
#include "patch.h"

using namespace kyotocabinet;

typedef std::function<int32_t(const PatchTreeKey&, const PatchTreeKey&)> comp;
// TODO: When two ids come from different dictionaries, find a correct way to compare them!!!!
auto comp_s = [] (const PatchTreeKey* e1, const PatchTreeKey* e2, const Dictionary* dict) {
  // If MSB is not set, id is HDT
  if (!(e1->get_subject() & 2147483648) && !(e2->get_subject() & 2147483648)) {
    return e1->get_subject  () - e2->get_subject  ();
  }

  //Else, translate to string and compare
  return e1->get_subject(dict).compare(e2->get_subject(dict));
};

auto comp_p = [] (const PatchTreeKey* e1, const PatchTreeKey* e2, const Dictionary* dict) {
  // If MSB is not set, id is HDT
  if (!(e1->get_predicate() & 2147483648) && !(e2->get_predicate() & 2147483648)) {
    return e1->get_predicate  () - e2->get_predicate  ();
  }

  //Else, translate to string and compare
  return e1->get_predicate(dict).compare(e2->get_predicate(dict));
};
auto comp_o = [] (const PatchTreeKey* e1, const PatchTreeKey* e2, const Dictionary* dict) {
  // If MSB is not set, id is HDT
  if (!(e1->get_object() & 2147483648) && !(e2->get_object() & 2147483648)) {
    return e1->get_object  () - e2->get_object  ();
  }

  //Else, translate to string and compare
  return e1->get_object(dict).compare(e2->get_object(dict));
};

// A PatchTreeKeyComparator can be used in a Kyoto Cabinet TreeDB for ordering by PatchTreeKey.
class PatchTreeKeyComparator : public Comparator {
protected:
    comp compare_1;
    comp compare_2;
    comp compare_3;
    Dictionary* dict;
public:
    static PatchTreeKeyComparator comparator_spo;
    PatchTreeKeyComparator(comp compare_1, comp compare_2, comp compare_3, Dictionary* dict);
    int32_t compare(const char* akbuf, size_t aksiz, const char* bkbuf, size_t bksiz);
    int32_t compare(const PatchTreeKey& key1, const PatchTreeKey& key2);
};

#endif //TPFPATCH_STORE_PATCH_TREE_KEY_COMPARATOR_H
