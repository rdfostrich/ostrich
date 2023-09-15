#include <iostream>
#include <kchashdb.h>
#include <HDT.hpp>

#include "../../main/cpp/patch/patch_tree.h"
#include "../../main/cpp/controller/controller.h"
#include "../../main/cpp/evaluate/evaluator.h"
#include "../../main/cpp/simpleprogresslistener.h"

#define BASEURI "<http://example.org>"

vector<string> split(string data, const string& token) {
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

void test_lookups_for_queries(Evaluator &evaluator, const string& queriesFilePath, int replications) {
    std::ifstream queriesFile(queriesFilePath);
    std::string line;
    std::cout << "---QUERIES START: " << queriesFilePath << "---" << std::endl;
    while (std::getline(queriesFile, line)) {
        std::vector<string> line_split = split(line, " ");
        evaluator.test_lookup(
                remove_brackets(line_split[0]),
                remove_brackets(line_split[1]),
                remove_brackets(line_split[2]),
                replications,
                line_split.size() > 4 ? std::stoi(line_split[3]) : 0, // offset
                line_split.size() > 4 ? std::stoi(line_split[4]) : -2 // limit
        );
    }
    std::cout << "---QUERIES END---" << std::endl;
}

void test_lookups_for_queries_ms(BearEvaluatorMS &evaluator, const string& queriesFilePath, int replications) {
    std::ifstream queriesFile(queriesFilePath);
    std::string line;
    std::cout << "---QUERIES START: " << queriesFilePath << "---" << std::endl;
    while (std::getline(queriesFile, line)) {
        std::vector<string> line_split = split(line, " ");
        evaluator.test_lookup(
                remove_brackets(line_split[0]),
                remove_brackets(line_split[1]),
                remove_brackets(line_split[2]),
                replications,
                line_split.size() > 4 ? std::stoi(line_split[3]) : 0, // offset
                -2 // limit
        );
    }
    std::cout << "---QUERIES END---" << std::endl;
}


int main(int argc, char *argv[]) {
    if (argc < 4 || argc > 9) {
        std::cerr << "Usage: " << argv[0] << " ingest|ingest-query|query|stats " << std::endl;
        std::cerr << "\tcmd \"ingest\": strategy strategy_parameter path_to_patches start_index end_index" << std::endl;
        std::cerr
                << "\tcmd \"ingest-query\": strategy strategy_parameter path_to_patches start_index end_index path_to_queries_file replications"
                << std::endl;
        std::cerr << "\tcmd \"query\": path_to_queries_file replications" << std::endl;
        return 1;
    }

    BearEvaluatorMS evaluator;
    SimpleProgressListener* listener = nullptr;

    if (std::strcmp("ingest", argv[1]) == 0 || std::strcmp("ingest-query", argv[1]) == 0) {
        std::string strategy_type = argv[2];
        std::string strategy_param = argv[3];
        SnapshotCreationStrategy* strategy = SnapshotCreationStrategy::get_composite_strategy(strategy_type, strategy_param);
        evaluator.init("./", argv[4], strategy, stoi(argv[5]), stoi(argv[6]), listener);
        if (std::strcmp("ingest-query", argv[1]) == 0) {
            test_lookups_for_queries_ms(evaluator, ((std::string) argv[7]), stoi(argv[8]));
        }
        delete strategy;
    } else if (std::strcmp("query", argv[1]) == 0) {
        evaluator.init_readonly("./", false);
        test_lookups_for_queries_ms(evaluator, ((std::string) argv[2]), stoi(argv[3]));
    } else if (std::strcmp("stats", argv[1]) == 0) {
        evaluator.init_readonly("./", false);
        evaluator.compute_statistics();
    }

    delete listener;

    evaluator.cleanup_controller();
}