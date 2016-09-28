#include <iostream>
#include <kchashdb.h>
#include <HDT.hpp>

#include "../../main/cpp/patch/patch_tree.h"
#include "../../main/cpp/controller/controller.h"
#include "../../main/cpp/evaluate/evaluator.h"
#include "../../main/cpp/simpleprogresslistener.h"

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

int main(int argc, char *argv[]) {
    if (argc != 5) {
        cerr << "Usage: " << argv[0] << " path_to_patches path_to_queries" << endl;
        exit(1);
    }

    Evaluator evaluator;
    evaluator.init("./", argv[1], stoi(argv[2]), stoi(argv[3]), new SimpleProgressListener());

    test_lookups_for_queries(evaluator, ((std::string) argv[4]) + "/subjectLookup/queries-sel-100-e0.1.txt");

    evaluator.cleanup_controller();
}
