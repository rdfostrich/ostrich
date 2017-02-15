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

void test_lookups_for_queries(Evaluator& evaluator, string queriesFilePath, TripleComponentRole queryType, int replications) {
    std::ifstream queriesFile(queriesFilePath);
    std::string line;
    cout << "---QUERIES START: " << queriesFilePath << "---" << endl;
    while (std::getline(queriesFile, line)) {
        vector<string> line_split = split(line, " ");
        std::string element = line_split[0];
        std::string s("");
        std::string p("");
        std::string o("");
        if (queryType == SUBJECT)   s = element;
        if (queryType == PREDICATE) p = element;
        if (queryType == OBJECT)    o = element;
        evaluator.test_lookup(s, p, o, replications);
    }
    cout << "---QUERIES END---" << endl;
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        cerr << "Usage: " << argv[0] << " path_to_patches start_index end_index [path_to_queries_file s|p|o replications]" << endl;
        exit(1);
    }

    Evaluator evaluator;
    SimpleProgressListener* listener = new SimpleProgressListener();
    evaluator.init("./", argv[1], stoi(argv[2]), stoi(argv[3]), listener);
    delete listener;

    if (argc >= 7) {
        TripleComponentRole queryType;
        std::string type(argv[5]);
        if      (type == "s") queryType = SUBJECT;
        else if (type == "p") queryType = PREDICATE;
        else if (type == "o") queryType = OBJECT;
        else {
            cerr << "Please provide a query type of s|p|o instead of " << type << endl;
            exit(1);
        }
        test_lookups_for_queries(evaluator, ((std::string) argv[4]), queryType, stoi(argv[6]));
    }

    evaluator.cleanup_controller();
}
