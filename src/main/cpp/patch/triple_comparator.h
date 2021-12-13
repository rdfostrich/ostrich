#ifndef OSTRICH_TRIPLE_COMPARATOR_H
#define OSTRICH_TRIPLE_COMPARATOR_H


#include <functional>
#include "../dictionary/dictionary_manager.h"
#include "triple.h"

constexpr size_t bitmask = 1ULL << (8 * sizeof(size_t) - 1);


typedef std::function<int32_t(const Triple& t1, const Triple& t2, std::shared_ptr<DictionaryManager> dict1, std::shared_ptr<DictionaryManager> dict2)> triplecomp;

triplecomp subject_comparator = [] (const Triple& t1, const Triple& t2, std::shared_ptr<DictionaryManager> dict1, std::shared_ptr<DictionaryManager> dict2) {
    size_t max_id = (size_t) -1;
    if (t1.get_subject() == max_id || t2.get_subject() == 0) return 1;
    if (t2.get_subject() == max_id || t1.get_subject() == 0) return -1;
    // the triples are using the same dictionary
    if (dict1 == dict2) {
        // If MSB is not set, id is HDT
        if (!(t1.get_subject() & bitmask) && !(t2.get_subject() & bitmask)) {
            return dict1->compareComponent(t1.get_subject(), t2.get_subject(), SUBJECT);
        }
    }
    //Else, translate to string and compare
    return t1.get_subject(*dict1).compare(t2.get_subject(*dict2));
};

triplecomp predicate_comparator = [] (const Triple& t1, const Triple& t2, std::shared_ptr<DictionaryManager> dict1, std::shared_ptr<DictionaryManager> dict2) {
    size_t max_id = (size_t) -1;
    if (t1.get_predicate() == max_id || t2.get_predicate() == 0) return 1;
    if (t2.get_predicate() == max_id || t1.get_predicate() == 0) return -1;
    if (dict1 == dict2) {
        // If MSB is not set, id is HDT
        if (!(t1.get_predicate() & bitmask) && !(t2.get_predicate() & bitmask)) {
            return dict1->compareComponent(t1.get_predicate(), t2.get_predicate(), PREDICATE);
        }
    }
    //Else, translate to string and compare
    return t1.get_predicate(*dict1).compare(t2.get_predicate(*dict2));
};

triplecomp object_comparator = [] (const Triple& t1, const Triple& t2, std::shared_ptr<DictionaryManager> dict1, std::shared_ptr<DictionaryManager> dict2) {
    size_t max_id = (size_t) -1;
    if (t1.get_object() == max_id || t2.get_object() == 0) return 1;
    if (t2.get_object() == max_id || t1.get_object() == 0) return -1;
    if (dict1 == dict2) {
        // If MSB is not set, id is HDT
        if (!(t1.get_object() & bitmask) && !(t2.get_object() & bitmask)) {
            return dict1->compareComponent(t1.get_object(), t2.get_object(), OBJECT);
        }
    }
    //Else, translate to string and compare
    return t1.get_object(*dict1).compare(t2.get_object(*dict2));
};

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


#endif //OSTRICH_TRIPLE_COMPARATOR_H
