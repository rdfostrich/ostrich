#include <iostream>
#include <kchashdb.h>
#include <unistd.h>
#include <cstring>

#include "../../main/cpp/patch/patch_tree.h"
#include "../../main/cpp/snapshot/snapshot_manager.h"
#include "../../main/cpp/snapshot/vector_triple_iterator.h"
#include "../../main/cpp/controller/controller.h"

#define BASEURI "<http://example.org>"


int main(int argc, char** argv) {
    if (argc < 6 || argc > 7) {
        std::cerr << "ERROR: Query command must be invoked as 'patch_id patch_id_end subject predicate object [offset]' " << std::endl;
        return 1;
    }

    // Load the store
    Controller controller("./", kyotocabinet::TreeDB::TCOMPRESS, true);

    // Get query parameters
    std::string s(argv[3]);
    std::string p(argv[4]);
    std::string o(argv[5]);
    if(std::strcmp(s.c_str(), "?") == 0) s = "";
    if(std::strcmp(p.c_str(), "?") == 0) p = "";
    if(std::strcmp(o.c_str(), "?") == 0) o = "";

    int patch_id_start = std::stoi(argv[1]);
    int patch_id_end = std::stoi(argv[2]);
    int offset = argc == 7 ? std::stoi(argv[6]) : 0;

    // Construct query
    StringTriple triple_pattern(s, p, o);

    std::pair<size_t, hdt::ResultEstimationType> count = controller.get_delta_materialized_count(triple_pattern, patch_id_start, patch_id_end, true);
    std::cerr << "Count: " << count.first << (count.second == hdt::EXACT ? "" : " (estimate)") << std::endl;

    TripleDeltaIterator* it = controller.get_delta_materialized(triple_pattern, offset, patch_id_start, patch_id_end);
    TripleDelta triple_delta;
    while (it->next(&triple_delta)) {
        std::cout << (triple_delta.is_addition() ? "+ " : "- ") << triple_delta.get_triple()->to_string(*(triple_delta.get_dictionary())) << std::endl;
    }
    delete it;

    return 0;
}
