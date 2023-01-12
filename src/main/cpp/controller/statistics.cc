#include "statistics.h"

Statistics::Statistics(Controller *controller): controller(controller) {}

double Statistics::change_ratio(int i, int j, bool allow_estimates) {
    auto count = controller->get_delta_materialized_count(StringTriple("", "", ""), i, j, allow_estimates);
    auto gi_size = controller->get_version_materialized_count(StringTriple("", "", ""), i, allow_estimates);
    double size_union = gi_size.first + count.first;
    return (double)count.first/size_union;
}

double Statistics::dynamicity(int i, int j) {
    std::unordered_set<std::string> terms;
    auto dm_it = controller->get_delta_materialized(StringTriple("", "", ""), 0, i, j);
    TripleDelta t;
    while (dm_it->next(&t)) {
        terms.insert(t.get_triple()->get_subject(*t.get_dictionary()));
        terms.insert(t.get_triple()->get_predicate(*t.get_dictionary()));
        terms.insert(t.get_triple()->get_object(*t.get_dictionary()));
    }
    delete dm_it;

    Triple t_vm;
    std::unordered_set<size_t> terms_i;
    auto vm_i = controller->get_version_materialized(StringTriple("", "", ""), 0, i);
    while (vm_i->next(&t_vm)) {
        terms_i.insert(t_vm.get_subject());
        terms_i.insert(t_vm.get_predicate());
        terms_i.insert(t_vm.get_object());
    }
    delete vm_i;

    std::unordered_set<size_t> terms_j;
    auto vm_j = controller->get_version_materialized(StringTriple("", "", ""), 0, j);
    while (vm_j->next(&t_vm)) {
        terms_j.insert(t_vm.get_subject());
        terms_j.insert(t_vm.get_predicate());
        terms_j.insert(t_vm.get_object());
    }
    delete vm_j;

    return (double)terms.size()/(double)(terms_i.size() + terms_j.size());
}

double Statistics::growth_ratio(int i, int j, bool allow_estimates) {
    auto count_i = controller->get_version_materialized_count(StringTriple("", "", ""), i, allow_estimates);
    auto count_j = controller->get_version_materialized_count(StringTriple("", "", ""), j, allow_estimates);
    return (double)count_j.first/(double)count_i.first;
}

void Statistics::sigma_ij(int i, int j, vector<std::string> &results) {
    Triple t;
    auto* si = new std::unordered_set<std::string>;
    auto vm_i = controller->get_version_materialized(StringTriple("", "", ""), 0, i);
    std::shared_ptr<DictionaryManager> dict_i = controller->get_dictionary_manager(i);
    while (vm_i->next(&t)) {
        si->insert(t.get_subject(*dict_i));
    }
    delete vm_i;

    auto* sj = new std::unordered_set<std::string>;
    auto vm_j = controller->get_version_materialized(StringTriple("", "", ""), 0, i);
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

double Statistics::triple_to_entity_change(int i, int j, bool allow_estimates) {
    std::vector<std::string> subjects_union;
    sigma_ij(i, j, subjects_union);

    size_t triple_count = 0;
    for (const auto& subject: subjects_union) {
        auto count = controller->get_delta_materialized_count(StringTriple(subject, "", ""), i, j, allow_estimates);
        triple_count += count.first;
    }

    return (double)triple_count/(double)subjects_union.size();
}

size_t Statistics::object_updates(int i, int j) {
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
                    update_count++;
                }
            }
        }
    }
    delete dm_it;

    return update_count;
}

size_t Statistics::orphan_additions(int i, int j) {
    return 0;
}

size_t Statistics::orphan_deletions(int i, int j) {
    return 0;
}
