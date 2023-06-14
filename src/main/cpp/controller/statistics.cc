#include "statistics.h"

Statistics::Statistics(Controller *controller): controller(controller) {}

double Statistics::change_ratio(int i, int j, bool allow_estimates) {
    auto count = controller->get_delta_materialized_count(StringTriple("", "", ""), i, j, allow_estimates);
    auto gi_size = controller->get_version_materialized_count(StringTriple("", "", ""), i, allow_estimates);
    double size_union = gi_size.first + count.first;
    return (double)count.first/size_union;
}

double Statistics::dynamicity(int i, int j) {
    Triple t_vm;
    std::shared_ptr<DictionaryManager> dict_i = controller->get_dictionary_manager(i);
    std::set<std::string> terms_i;
    auto vm_i = controller->get_version_materialized(StringTriple("", "", ""), 0, i);
    while (vm_i->next(&t_vm)) {
        terms_i.insert(t_vm.get_subject(*dict_i));
        terms_i.insert(t_vm.get_predicate(*dict_i));
        terms_i.insert(t_vm.get_object(*dict_i));
    }
    delete vm_i;

    std::shared_ptr<DictionaryManager> dict_j = controller->get_dictionary_manager(j);
    std::set<std::string> terms_j;
    auto vm_j = controller->get_version_materialized(StringTriple("", "", ""), 0, j);
    while (vm_j->next(&t_vm)) {
        terms_j.insert(t_vm.get_subject(*dict_j));
        terms_j.insert(t_vm.get_predicate(*dict_j));
        terms_j.insert(t_vm.get_object(*dict_j));
    }
    delete vm_j;

    std::vector<std::string> added_terms;
    std::set_difference(terms_j.begin(), terms_j.end(), terms_i.begin(), terms_i.end(), std::back_inserter(added_terms));
    std::vector<std::string> deleted_terms;
    std::set_difference(terms_i.begin(), terms_i.end(), terms_j.begin(), terms_j.end(), std::back_inserter(deleted_terms));

    std::vector<std::string> changed_terms_union;
    std::set_union(added_terms.begin(), added_terms.end(), deleted_terms.begin(), deleted_terms.end(), std::back_inserter(changed_terms_union));

    std::vector<std::string> all_terms;
    std::set_union(terms_i.begin(), terms_i.end(), terms_j.begin(), terms_j.end(), std::back_inserter(all_terms));

    return (double)changed_terms_union.size()/(double)(all_terms.size());
}

double Statistics::growth_ratio(int i, int j, bool allow_estimates) {
    int count_i = 0;
    Triple t;
    auto vm_i = controller->get_version_materialized(StringTriple("", "", ""), 0 , i);
    while (vm_i->next(&t)) {
        count_i++;
    }
    delete vm_i;
    int count_j = 0;
    auto vm_j = controller->get_version_materialized(StringTriple("", "", ""), 0, j);
    while (vm_j->next(&t)) {
        count_j++;
    }
    delete vm_j;
    return (double)count_j/(double)count_i;
}

void Statistics::sigma_ij(int i, int j, std::vector<std::string> &results) {
    Triple t;
    auto* si = new std::set<std::string>;
    auto vm_i = controller->get_version_materialized(StringTriple("", "", ""), 0, i);
    std::shared_ptr<DictionaryManager> dict_i = controller->get_dictionary_manager(i);
    while (vm_i->next(&t)) {
        si->insert(t.get_subject(*dict_i));
    }
    delete vm_i;

    auto* sj = new std::set<std::string>;
    auto vm_j = controller->get_version_materialized(StringTriple("", "", ""), 0, j);
    std::shared_ptr<DictionaryManager> dict_j = controller->get_dictionary_manager(j);
    while (vm_j->next(&t)) {
        sj->insert(t.get_subject(*dict_j));
    }
    delete vm_j;

    std::vector<std::string> added_subjects;
    std::set_difference(sj->begin(), sj->end(), si->begin(), si->end(), std::back_inserter(added_subjects));
    std::vector<std::string> deleted_subjects;
    std::set_difference(si->begin(), si->end(), sj->begin(), sj->end(), std::back_inserter(deleted_subjects));
    delete si;
    delete sj;

    std::set_union(added_subjects.begin(), added_subjects.end(), deleted_subjects.begin(), deleted_subjects.end(), std::back_inserter(results));
}

// High level changes
size_t Statistics::entity_changes(int i, int j) {
    std::vector<std::string> subjects_union;
    sigma_ij(i, j, subjects_union);
    return subjects_union.size();
}

double Statistics::triple_to_entity_change(int i, int j, std::set<StringTriple>* consumed_triples, bool allow_estimates) {
    std::vector<std::string> subjects_union;
    sigma_ij(i, j, subjects_union);
    if (subjects_union.empty()) {
        return 0;
    }

    size_t triple_count = 0;
    for (const auto& subject: subjects_union) {
        auto dm_it = controller->get_delta_materialized(StringTriple(subject, "", ""), 0, i, j);
        TripleDelta td;
        while (dm_it->next(&td)) {
            if (consumed_triples != nullptr) {
                std::string s = td.get_triple()->get_subject(*td.get_dictionary());
                std::string p = td.get_triple()->get_predicate(*td.get_dictionary());
                std::string o = td.get_triple()->get_object(*td.get_dictionary());
                auto find_it = consumed_triples->find(StringTriple(s, p, o));
                if (find_it == consumed_triples->end()) {
                    consumed_triples->emplace(s, p, o);
                    triple_count++;
                }
            } else {
                triple_count++;
            }
        }
    }

    return (double)triple_count/(double)subjects_union.size();
}

size_t Statistics::object_updates(int i, int j, std::set<StringTriple>* consumed_triples) {
    size_t update_count = 0;

    auto dm_it = controller->get_delta_materialized(StringTriple("", "", ""), 0, i, j);
    TripleDelta td;
    while (dm_it->next(&td)) {
        if (!td.is_addition()) {
            std::string s = td.get_triple()->get_subject(*td.get_dictionary());
            std::string p = td.get_triple()->get_predicate(*td.get_dictionary());
            std::string o = td.get_triple()->get_object(*td.get_dictionary());
            auto replacement_lookup = controller->get_delta_materialized(StringTriple(s, p, ""), 0, i, j);
            TripleDelta td2;
            while (replacement_lookup->next(&td2)) {
                if (td2.is_addition() && (td2.get_triple()->get_object(*td2.get_dictionary()) != o)) {
                    if (consumed_triples != nullptr) {
                        std::string s2 = td2.get_triple()->get_subject(*td2.get_dictionary());
                        std::string p2 = td2.get_triple()->get_predicate(*td2.get_dictionary());
                        std::string o2 = td2.get_triple()->get_object(*td2.get_dictionary());
                        auto find_it = consumed_triples->find(StringTriple(s2, p2, o2));
                        if (find_it == consumed_triples->end()) {
                            consumed_triples->emplace(s2, p2, o2);
                            update_count++;
                        }
                    } else {
                        update_count++;
                    }
                }
            }
            delete replacement_lookup;
        }
//        std::cout << td.get_triple()->to_string(*td.get_dictionary()) << " " << td.is_addition() << std::endl;
    }
    delete dm_it;

    return update_count;
}

size_t Statistics::orphan_additions(int i, int j, std::set<StringTriple>* consumed_triples) {
    return 0;
}

size_t Statistics::orphan_deletions(int i, int j, std::set<StringTriple>* consumed_triples) {
    return 0;
}
