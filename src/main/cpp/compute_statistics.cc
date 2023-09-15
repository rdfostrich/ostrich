#include <iostream>

#include "./controller/statistics.h"


int main(int argc, char** argv) {
    if (argc != 4) {
        std::cerr << "ERROR: Statistics command must be invoked as 'statistic_name version_i version_j' " << std::endl;
        std::cerr << "\tstatistic_name can be [change_ratio|dynamicity|growth_ratio|entity_changes|triple_to_entity_change|object_updates]" << std::endl;
        return 1;
    }

    // Load the store
    Controller controller("./", kyotocabinet::TreeDB::TCOMPRESS, true);

    // Build statistic class
    Statistics statistics(&controller);

    // Get query parameters
    std::string statistic_name(argv[1]);
    int i = std::stoi(argv[2]);
    int j = std::stoi(argv[3]);

    if (statistic_name == "change_ratio") {
        std::cout << "Change-ratio (" << i << ", " << j << ")" << std::endl;
        double cr = statistics.change_ratio(i, j);
        std::cout << cr << std::endl;
    } else if (statistic_name == "dynamicity") {
        std::cout << "Dynamicity (" << i << ", " << j << ")" << std::endl;
        double d = statistics.dynamicity(i, j);
        std::cout << d << std::endl;
    } else if (statistic_name == "growth_ratio") {
        std::cout << "Growth-ratio (" << i << ", " << j << ")" << std::endl;
        double gr = statistics.growth_ratio(i, j);
        std::cout << gr << std::endl;
    } else if (statistic_name == "entity_changes") {
        std::cout << "Entity-changes (" << i << ", " << j << ")" << std::endl;
        size_t etc = statistics.entity_changes(i, j);
        std::cout << etc << std::endl;
    } else if (statistic_name == "triple_to_entity_change") {
        std::cout << "Triple-to-entity-change (" << i << ", " << j << ")" << std::endl;
        double tec = statistics.triple_to_entity_change(i, j);
        std::cout << tec << std::endl;
    } else if (statistic_name == "object_updates") {
        std::cout << "Object-updates (" << i << ", " << j << ")" << std::endl;
        size_t obj_u = statistics.object_updates(i, j);
        std::cout << obj_u << std::endl;
    } else {
        std::cerr << "Unknown statistic name parameter: " << statistic_name << std::endl;
    }

    return 0;
}
