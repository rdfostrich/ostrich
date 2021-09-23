#include <iostream>
#include <vector>
#include <kchashdb.h>
#include <rdf/RDFParserNtriples.hpp>

#include "../../main/cpp/patch/patch_tree.h"
#include "../../main/cpp/snapshot/snapshot_manager.h"
#include "../../main/cpp/controller/controller.h"
#include "snapshot/combined_triple_iterator.h"
#include "simpleprogresslistener.h"

#define BASEURI "<http://example.org>"

using namespace std;

int main(int argc, char** argv) {
    if (argc < 2) {
        cerr << "ERROR: Insert command must be invoked as '[-v] [-s int] patch_id [+|- file_1.nt [file_2.nt [...]]]*' " << endl;
        return 1;
    }

    int param_offset = 0;

    bool verbose = std::string(argv[1]) == "-v";
    param_offset += 1;
    ProgressListener* progressListener = verbose ? new SimpleProgressListener() : nullptr;

    bool has_strat = std::string(argv[1 + param_offset]) == "-s";
    unsigned strat_value = 0;
    if (has_strat) {
        param_offset += 1;
        strat_value = std::stoul(argv[1 + param_offset]);
        param_offset += 1;
    }

    // Load the store

    // Should be moved into a strategy factory ?
    SnapshotCreationStrategy* strategy = nullptr;
    if (has_strat) {
        if (strat_value == 0) {
            strategy = new NeverCreateSnapshot;
        } else if (strat_value == 1) {
            strategy = new AlwaysCreateSnapshot;
        } else {
            strategy = new CreateSnapshotEveryN(strat_value);
        }
    }

    Controller controller("./", strategy, TreeDB::TCOMPRESS);

    // Get parameters
    int patch_id = std::stoi(argv[1 + param_offset]);

    // Read command line parameters
    std::vector<std::pair<IteratorTripleString*, bool>> files;
    bool additions = true;
    for (int file_id = 2 + param_offset; file_id < argc; file_id++) {
        std::string file(argv[file_id]);
        if (file == "+" || file == "-") {
            additions = file == "+";
        } else {
            ifstream f(file.c_str());
            if (!f.good()) {
                cerr << "Could not find a file at location: " << file << endl;
                return 1;
            }
            IteratorTripleString *file_it = new RDFParserNtriples(file.c_str(), NTRIPLES);
            files.emplace_back(file_it, additions);
        }
    }

    bool status = controller.ingest(files, patch_id, progressListener);

    if (progressListener)
        std::cout << std::endl;

    delete progressListener;
    delete strategy;

    return status;
}

