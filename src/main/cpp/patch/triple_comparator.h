#ifndef OSTRICH_TRIPLE_COMPARATOR_H
#define OSTRICH_TRIPLE_COMPARATOR_H


#include <functional>
#include "../dictionary/dictionary_manager.h"
#include "triple.h"


typedef std::function<int32_t(const Triple& t1, const Triple& t2, std::shared_ptr<DictionaryManager> dict1, std::shared_ptr<DictionaryManager> dict2)> triplecomp;

extern triplecomp subject_comparator;
extern triplecomp predicate_comparator;
extern triplecomp object_comparator;


class TripleComparator {
private:
    triplecomp comp1;
    triplecomp comp2;
    triplecomp comp3;

    std::shared_ptr<DictionaryManager> dict1;
    std::shared_ptr<DictionaryManager> dict2;
public:
    TripleComparator(triplecomp comp1, triplecomp comp2, triplecomp comp3, std::shared_ptr<DictionaryManager> dict1, std::shared_ptr<DictionaryManager> dict2);
    int compare(const Triple& triple1, const Triple& triple2) const;
    bool operator()(const Triple& triple1, const Triple& triple2) const;

    void set_dictionary1(std::shared_ptr<DictionaryManager> dict);
    void set_dictionary2(std::shared_ptr<DictionaryManager> dict);
};


TripleComparator get_triple_comparator(TripleComponentOrder order);


#endif //OSTRICH_TRIPLE_COMPARATOR_H
