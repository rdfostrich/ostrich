#include "triple_comparator.h"

TripleComparator::TripleComparator(triplecomp comp1, triplecomp comp2, triplecomp comp3, std::shared_ptr<DictionaryManager> dict1, std::shared_ptr<DictionaryManager> dict2): comp1(comp1), comp2(comp2), comp3(comp3), dict1(dict1), dict2(dict2) {}

int TripleComparator::compare(const Triple &triple1, const Triple &triple2) const {
    int comp = comp1(triple1, triple2, dict1, dict2);
    if(comp == 0) {
        comp = comp2(triple1, triple2, dict1, dict2);
        if(comp == 0) {
            comp = comp3(triple1, triple2, dict1, dict2);
        }
    }
    return comp;
}

bool TripleComparator::operator()(const Triple &triple1, const Triple &triple2) const {
    return compare(triple1, triple2);
}

void TripleComparator::set_dictionary1(std::shared_ptr<DictionaryManager> dict) {
    dict1 = std::move(dict);
}

void TripleComparator::set_dictionary2(std::shared_ptr<DictionaryManager> dict) {
    dict2 = std::move(dict);
}

