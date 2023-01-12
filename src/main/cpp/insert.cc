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


int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "ERROR: Insert command must be invoked as '[-v] [-s string int|float] patch_id [+|- file_1.nt [file_2.nt [...]]]*' " << std::endl;
        return 1;
    }

    int param_offset = 0;

    bool verbose = std::string(argv[1]) == "-v";
    if (verbose) {
        param_offset += 1;
    }
    hdt::ProgressListener* progressListener = verbose ? new SimpleProgressListener() : nullptr;

    SnapshotCreationStrategy* strategy = nullptr;
    bool has_strat = std::string(argv[1 + param_offset]) == "-s";
    if (has_strat) {
        param_offset += 1;
        std::string strat_name = argv[1 + param_offset];
        param_offset += 1;
        std::string strat_param = argv[1 + param_offset];
        param_offset += 1;
        strategy = SnapshotCreationStrategy::get_composite_strategy(strat_name, strat_param);
    }

    // Load the store

    Controller controller("./", strategy, kyotocabinet::TreeDB::TCOMPRESS);

    // Get parameters
    int patch_id = std::stoi(argv[1 + param_offset]);

    // Read command line parameters
    std::vector<std::pair<hdt::IteratorTripleString*, bool>> files;
    bool additions = true;
    for (int file_id = 2 + param_offset; file_id < argc; file_id++) {
        std::string file(argv[file_id]);
        if (file == "+" || file == "-") {
            additions = file == "+";
        } else {
            ifstream f(file.c_str());
            if (!f.good()) {
                std::cerr << "Could not find a file at location: " << file << endl;
                return 1;
            }
            hdt::IteratorTripleString *file_it = new hdt::RDFParserNtriples(file.c_str(), hdt::NTRIPLES);
            files.emplace_back(file_it, additions);
        }
    }

    bool status = controller.ingest(files, patch_id, true, progressListener);

    if (progressListener)
        std::cout << std::endl;

    delete progressListener;
    delete strategy;

    return status;
}

