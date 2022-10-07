#include "triple_comparator.h"


triplecomp subject_comparator = [] (const Triple& t1, const Triple& t2, std::shared_ptr<DictionaryManager> dict1, std::shared_ptr<DictionaryManager> dict2) {
    size_t max_id = std::numeric_limits<size_t>::max();
    if (dict1 == nullptr || dict2 == nullptr) return (int32_t)(t1.get_subject() - t2.get_subject());
    if (t1.get_subject() == max_id || t2.get_subject() == 0) return 1;
    if (t2.get_subject() == max_id || t1.get_subject() == 0) return -1;
    // The triples are using the same dictionary
    if (dict1 == dict2) {
        return dict1->compareComponent(t1.get_subject(), t2.get_subject(), hdt::SUBJECT);
    }
    //Else, translate to string and compare
    return t1.get_subject(*dict1).compare(t2.get_subject(*dict2));
};

triplecomp predicate_comparator = [] (const Triple& t1, const Triple& t2, std::shared_ptr<DictionaryManager> dict1, std::shared_ptr<DictionaryManager> dict2) {
    size_t max_id = std::numeric_limits<size_t>::max();
    if (dict1 == nullptr || dict2 == nullptr) return (int32_t)(t1.get_predicate() - t2.get_predicate());
    if (t1.get_predicate() == max_id || t2.get_predicate() == 0) return 1;
    if (t2.get_predicate() == max_id || t1.get_predicate() == 0) return -1;
    // The triples are using the same dictionary
    if (dict1 == dict2) {
        return dict1->compareComponent(t1.get_predicate(), t2.get_predicate(), hdt::PREDICATE);
    }
    //Else, translate to string and compare
    return t1.get_predicate(*dict1).compare(t2.get_predicate(*dict2));
};

triplecomp object_comparator = [] (const Triple& t1, const Triple& t2, std::shared_ptr<DictionaryManager> dict1, std::shared_ptr<DictionaryManager> dict2) {
    size_t max_id = std::numeric_limits<size_t>::max();
    if (dict1 == nullptr || dict2 == nullptr) return (int32_t)(t1.get_object() - t2.get_object());
    if (t1.get_object() == max_id || t2.get_object() == 0) return 1;
    if (t2.get_object() == max_id || t1.get_object() == 0) return -1;
    // The triples are using the same dictionary
    if (dict1 == dict2) {
        return dict1->compareComponent(t1.get_object(), t2.get_object(), hdt::OBJECT);
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

int TripleComparator::compare(const Triple &triple1, const Triple &triple2, std::shared_ptr<DictionaryManager> dict_1,
                              std::shared_ptr<DictionaryManager> dict_2) const {
    int comp = comp1(triple1, triple2, dict_1, dict_2);
    if(comp == 0) {
        comp = comp2(triple1, triple2, dict_1, dict_2);
        if(comp == 0) {
            comp = comp3(triple1, triple2, dict_1, dict_2);
        }
    }
    return comp;
}

int TripleComparator::compare(const hdt::TripleID &triple1, const hdt::TripleID &triple2, std::shared_ptr<DictionaryManager> dict_1,
                          std::shared_ptr<DictionaryManager> dict_2) const {
    Triple t1(triple1.getSubject(), triple1.getPredicate(), triple1.getObject());
    Triple t2(triple2.getSubject(), triple2.getPredicate(), triple2.getObject());
    return compare(t1, t2, std::move(dict_1), std::move(dict_2));
}

int TripleComparator::compare(const TripleDelta *triple1, const TripleDelta *triple2) const {
    return compare(*(triple1->get_triple_const()), *(triple2->get_triple_const()), triple1->get_dictionary(), triple2->get_dictionary());
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

TripleComparator* TripleComparator::get_triple_comparator(hdt::TripleComponentOrder order, std::shared_ptr<DictionaryManager> dict1, std::shared_ptr<DictionaryManager> dict2) {
    switch (order) {
        case hdt::SPO:
            return new TripleComparator(subject_comparator, predicate_comparator, object_comparator, dict1, dict2);
        case hdt::SOP:
            return new TripleComparator(subject_comparator, object_comparator, predicate_comparator, dict1, dict2);
        case hdt::PSO:
            return new TripleComparator(predicate_comparator, subject_comparator, object_comparator, dict1, dict2);
        case hdt::POS:
            return new TripleComparator(predicate_comparator, object_comparator, subject_comparator, dict1, dict2);
        case hdt::OSP:
            return new TripleComparator(object_comparator, subject_comparator, predicate_comparator, dict1, dict2);
        case hdt::OPS:
            return new TripleComparator(object_comparator, predicate_comparator, subject_comparator, dict1, dict2);
        case hdt::Unknown:
            return new TripleComparator(subject_comparator, predicate_comparator, object_comparator, dict1, dict2);
    }
    return new TripleComparator(subject_comparator, predicate_comparator, object_comparator, dict1, dict2);
}

