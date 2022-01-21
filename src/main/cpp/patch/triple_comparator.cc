#include "triple_comparator.h"

constexpr size_t bitmask = 1ULL << (8 * sizeof(size_t) - 1);


triplecomp subject_comparator = [] (const Triple& t1, const Triple& t2, std::shared_ptr<DictionaryManager> dict1, std::shared_ptr<DictionaryManager> dict2) {
    size_t max_id = (size_t) -1;
    if (dict1 == nullptr || dict2 == nullptr) return (int32_t)(t1.get_predicate() - t2.get_predicate());
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
    if (dict1 == nullptr || dict2 == nullptr) return (int32_t)(t1.get_predicate() - t2.get_predicate());
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
    if (dict1 == nullptr || dict2 == nullptr) return (int32_t)(t1.get_predicate() - t2.get_predicate());
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

int TripleComparator::compare(const Triple &triple1, const Triple &triple2, std::shared_ptr<DictionaryManager> dict1,
                              std::shared_ptr<DictionaryManager> dict2) const {
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
    return compare(triple1, triple2) < 0;
}

bool TripleComparator::operator()(const TripleVersions *triple1, const TripleVersions *triple2) const {
    return compare(*(triple1->get_triple_const()), *(triple2->get_triple_const()), triple1->get_dictionary(), triple2->get_dictionary()) < 0;
}

bool TripleComparator::operator()(const TripleDelta *triple1, const TripleDelta *triple2) const {
    return compare(*(triple1->get_triple_const()), *(triple2->get_triple_const()), triple1->get_dictionary(), triple2->get_dictionary()) < 0;
}

void TripleComparator::set_dictionary1(std::shared_ptr<DictionaryManager> dict) {
    dict1 = std::move(dict);
}

void TripleComparator::set_dictionary2(std::shared_ptr<DictionaryManager> dict) {
    dict2 = std::move(dict);
}

TripleComparator* TripleComparator::get_triple_comparator(TripleComponentOrder order, std::shared_ptr<DictionaryManager> dict1, std::shared_ptr<DictionaryManager> dict2) {
    switch (order) {
        case SPO:
            return new TripleComparator(subject_comparator, predicate_comparator, object_comparator, dict1, dict2);
        case SOP:
            return new TripleComparator(subject_comparator, object_comparator, predicate_comparator, dict1, dict2);
        case PSO:
            return new TripleComparator(predicate_comparator, subject_comparator, object_comparator, dict1, dict2);
        case POS:
            return new TripleComparator(predicate_comparator, object_comparator, subject_comparator, dict1, dict2);
        case OSP:
            return new TripleComparator(object_comparator, subject_comparator, predicate_comparator, dict1, dict2);
        case OPS:
            return new TripleComparator(object_comparator, predicate_comparator, subject_comparator, dict1, dict2);
        case Unknown:
            return new TripleComparator(subject_comparator, predicate_comparator, object_comparator, dict1, dict2);
    }
    return new TripleComparator(subject_comparator, predicate_comparator, object_comparator, dict1, dict2);
}
