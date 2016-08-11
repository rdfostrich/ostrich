#include <iostream>
#include <kchashdb.h>
#include <util/StopWatch.hpp>
#include <rdf/RDFParserNtriples.hpp>
#include <dirent.h>
#include <HDT.hpp>

#include "../../main/cpp/patch/patch_tree.h"
#include "../../main/cpp/snapshot/snapshot_manager.h"
#include "../../main/cpp/snapshot/vector_triple_iterator.h"
#include "../../main/cpp/controller/controller.h"
#include "../../main/cpp/snapshot/combined_triple_iterator.h"

#define BASEURI "<http://example.org>"

using namespace std;
using namespace kyotocabinet;

vector<string> split(string data, string token) {
    vector<string> output;
    size_t pos = string::npos; // size_t to avoid improbable overflow
    do {
        pos = data.find(token);
        output.push_back(data.substr(0, pos));
        if (string::npos != pos)
            data = data.substr(pos + token.size());
    } while (string::npos != pos);
    return output;
}

void test_lookups_for_queries(Evaluator& evaluator, string queriesFilePath) {
    std::ifstream queriesFile(queriesFilePath);
    std::string line;
    while (std::getline(queriesFile, line)) {
        vector<string> line_split = split(line, " ");
        std::string element = line_split[0];
        evaluator.test_lookup(element, "", "");
    }
}

int main() {
    // TODO: don't hardcode path
    Evaluator evaluator;
    evaluator.init("/Users/rtaelman/nosync/patch-generator/bear/patches");

    test_lookups_for_queries(evaluator, "/Users/rtaelman/nosync/BEAR/queries/subjectLookup/queries-sel-100-e0.1.txt");

    evaluator.cleanup_controller();
}
