#ifndef OSTRICH_STATISTICS_H
#define OSTRICH_STATISTICS_H


#include "controller.h"

/**
 * Class to compute various statistics over an Ostrich store.
 * The statistics are described in detailed in this research paper: https://doi.org/10.3233/SW-210434
 */
class Statistics {
private:
    Controller* controller;

    void sigma_ij(int i, int j, std::vector<std::string>& results);

public:
    explicit Statistics(Controller* controller);

    // Low level changes
    double change_ratio(int i, int j, bool allow_estimates = false);
    double dynamicity(int i, int j);
    double growth_ratio(int i, int j, bool allow_estimates = false);

    // High level changes
    size_t entity_changes(int i, int j);
    double triple_to_entity_change(int i, int j, bool allow_estimates = false);
    size_t object_updates(int i, int j);
    size_t orphan_additions(int i, int j);
    size_t orphan_deletions(int i, int j);
};


#endif //OSTRICH_STATISTICS_H
