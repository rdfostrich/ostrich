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
    if (argc < 4 || argc > 5) {
        cerr << "ERROR: Query command must be invoked as 'subject predicate object [offset]' " << endl;
        return 1;
    }

    // Load the store
    Controller controller("./", TreeDB::TCOMPRESS, true);

    // Get query parameters
    std::string s(argv[1]);
    std::string p(argv[2]);
    std::string o(argv[3]);
    if(std::strcmp(s.c_str(), "?") == 0) s = "";
    if(std::strcmp(p.c_str(), "?") == 0) p = "";
    if(std::strcmp(o.c_str(), "?") == 0) o = "";

    int offset = argc == 5 ? std::stoi(argv[4]) : 0;

    // Construct query
    StringTriple triple_pattern(s, p, o);

    std::pair<size_t, ResultEstimationType> count = controller.get_version_count(triple_pattern, true);
    cerr << "Count: " << count.first << (count.second == EXACT ? "" : " (estimate)") << endl;

    TripleVersionsIteratorCombined* it = controller.get_version(triple_pattern, offset);
    TripleVersionsString triple_versions;
    while (it->next(&triple_versions)) {
        std::stringstream vect;
        std::copy(triple_versions.get_versions()->begin(), triple_versions.get_versions()->end(), std::ostream_iterator<int>(vect, " "));
        cout << triple_versions.get_triple()->to_string() << " :: [ " << vect.str() << "]" << endl;
    }
    delete it;

    return 0;
}
