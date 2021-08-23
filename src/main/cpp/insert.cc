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
        cerr << "ERROR: Insert command must be invoked as '[-v] patch_id [+|- file_1.nt [file_2.nt [...]]]*' " << endl;
        return 1;
    }

    bool verbose = std::string(argv[1]) == "-v";
    ProgressListener* progressListener = verbose ? new SimpleProgressListener() : nullptr;

    // Load the store
    Controller controller("./", TreeDB::TCOMPRESS);

    // Get parameters
    int patch_id = std::atoi(argv[1 + verbose]);


    // Read command line parameters
    std::vector<std::pair<IteratorTripleString*, bool>> files;
    bool additions = true;
    for (int file_id = 2 + verbose; file_id < argc; file_id++) {
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

    SnapshotCreationStrategy* strat = new NeverCreateSnapshot;
    bool status = controller.ingest(files, patch_id, *strat, progressListener);

    delete strat;

    return status;
}

