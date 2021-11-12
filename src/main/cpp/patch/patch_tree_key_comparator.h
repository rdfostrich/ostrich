#ifndef TPFPATCH_STORE_PATCH_TREE_KEY_COMPARATOR_H
#define TPFPATCH_STORE_PATCH_TREE_KEY_COMPARATOR_H

#include <kchashdb.h>
#include <Dictionary.hpp>
#include "triple.h"
#include "../dictionary/dictionary_manager.h"

using namespace kyotocabinet;

typedef std::function<int32_t(const PatchTreeKey&, const PatchTreeKey&, DictionaryManager& dict)> comp;
extern comp comp_s;
extern comp comp_p;
extern comp comp_o;

// A PatchTreeKeyComparator can be used in a Kyoto Cabinet TreeDB for ordering by PatchTreeKey.
class PatchTreeKeyComparator : public Comparator {
protected:
    comp compare_1;
    comp compare_2;
    comp compare_3;
    std::shared_ptr<DictionaryManager> dict;
public:
    PatchTreeKeyComparator(comp compare_1, comp compare_2, comp compare_3, std::shared_ptr<DictionaryManager> dict);
    int32_t compare(const char* akbuf, size_t aksiz, const char* bkbuf, size_t bksiz);
    int32_t compare(const PatchTreeKey& key1, const PatchTreeKey& key2);
};

#endif //TPFPATCH_STORE_PATCH_TREE_KEY_COMPARATOR_H
