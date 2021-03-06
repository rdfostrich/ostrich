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

string remove_brackets(string element) {
    if (element.at(0) == '<') {
        return element.substr(1, element.size() - 2);
    }
    if (element.at(0) == '?') {
        return "";
    }
    return element;
}

void test_lookups_for_queries(Evaluator& evaluator, string queriesFilePath, int replications) {
    std::ifstream queriesFile(queriesFilePath);
    std::string line;
    cout << "---QUERIES START: " << queriesFilePath << "---" << endl;
    while (std::getline(queriesFile, line)) {
        vector<string> line_split = split(line, " ");
        evaluator.test_lookup(
                remove_brackets(line_split[0]),
                remove_brackets(line_split[1]),
                remove_brackets(line_split[2]),
                replications,
                line_split.size() > 4 ? std::atoi(line_split[3].c_str()) : 0, // offset
                line_split.size() > 4 ? std::atoi(line_split[4].c_str()) : -2 // limit
        );
    }
    cout << "---QUERIES END---" << endl;
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        cerr << "Usage: " << argv[0] << " path_to_patches start_index end_index [path_to_queries_file replications]" << endl;
        exit(1);
    }

    Evaluator evaluator;
    SimpleProgressListener* listener = new SimpleProgressListener();
    evaluator.init("./", argv[1], stoi(argv[2]), stoi(argv[3]), listener);
    delete listener;

    if (argc >= 6) {
        test_lookups_for_queries(evaluator, ((std::string) argv[4]), stoi(argv[5]));
    }

    evaluator.cleanup_controller();
}
