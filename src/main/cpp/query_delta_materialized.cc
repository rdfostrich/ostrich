#include <iostream>
#include <kchashdb.h>
#include <unistd.h>
#include <cstring>

#include "../../main/cpp/patch/patch_tree.h"
#include "../../main/cpp/snapshot/snapshot_manager.h"
#include "../../main/cpp/snapshot/vector_triple_iterator.h"
#include "../../main/cpp/controller/controller.h"

#define BASEURI "<http://example.org>"

using namespace std;
using namespace kyotocabinet;

int main(int argc, char** argv) {
    if (argc < 6 || argc > 7) {
        cerr << "ERROR: Query command must be invoked as 'patch_id_start patch_id_end subject predicate object [offset]' " << endl;
        return 1;
    }

    // Load the store
    Controller controller("./", TreeDB::TCOMPRESS, true);

    // Get query parameters
    std::string s(argv[3]);
    std::string p(argv[4]);
    std::string o(argv[5]);
    if(std::strcmp(s.c_str(), "?") == 0) s = "";
    if(std::strcmp(p.c_str(), "?") == 0) p = "";
    if(std::strcmp(o.c_str(), "?") == 0) o = "";

    int patch_id_start = std::atoi(argv[1]);
    int patch_id_end = std::atoi(argv[2]);
    int offset = argc == 7 ? std::atoi(argv[6]) : 0;

    // Construct query
    std::shared_ptr<DictionaryManager> dict = controller.get_dictionary_manager(patch_id_start);
    Triple triple_pattern(s, p, o, dict);

    std::pair<size_t, ResultEstimationType> count = controller.get_delta_materialized_count(triple_pattern, patch_id_start, patch_id_end, true);
    cerr << "Count: " << count.first << (count.second == EXACT ? "" : " (estimate)") << endl;

    TripleDeltaIterator* it = controller.get_delta_materialized(triple_pattern, offset, patch_id_start, patch_id_end);
    TripleDelta triple_delta;
    while (it->next(&triple_delta)) {
        cout << (triple_delta.is_addition() ? "+ " : "- ") << triple_delta.get_triple()->to_string(*dict) << endl;
    }
    delete it;

    return 0;
}
