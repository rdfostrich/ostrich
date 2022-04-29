#ifndef TPFPATCH_STORE_EVALUATOR_H
#define TPFPATCH_STORE_EVALUATOR_H


#include "../controller/controller.h"

#define BASEURI "<http://example.org>"

const char k_path_separator =
#ifdef _WIN32
        '\\';
#else
        '/';
#endif

class Evaluator {
private:
    int patch_count = 0;
    Controller* controller;
public:
    void init(string basePath, string patchesBasePatch, int startIndex, int endIndex, ProgressListener* progressListener = NULL);
    void test_lookup(string s, string p, string o, int replications, int offset, int limit);
    void cleanup_controller();
protected:
    void populate_controller_with_version(int patch_id, string path, ProgressListener* progressListener = NULL);
    std::ifstream::pos_type patchstore_size(Controller* controller);
    std::ifstream::pos_type filesize(string file);
    IteratorTripleString* get_from_file(string file);
    long long measure_lookup_version_materialized(Dictionary& dict, Triple triple_pattern, int offset, int patch_id, int limit, int replications, int& result_count);
    long long measure_count_version_materialized(Triple triple_pattern, int patch_id, int replications);
    long long measure_lookup_delta_materialized(Dictionary& dict, Triple triple_pattern, int offset, int patch_id_start, int patch_id_end, int limit, int replications, int& result_count);
    long long measure_count_delta_materialized(Triple triple_pattern, int patch_id_start, int patch_id_end, int replications);
    long long measure_lookup_version(Dictionary& dict, Triple triple_pattern, int offset, int limit, int replications, int& result_count);
    long long measure_count_version(Triple triple_pattern, int replications);
};


// Class to evaluate with multiple snapshots context
// Needs two folders:
// Folder "IC" which contains at least one instance of the dataset (use for initial version)
// the name of the first revision should be "1.nt"
// Folder "CB" which contains the patches
// the names for patches should be "data-added_v1-v2.nt" and "data-deleted_v1-v2.nt"
class BearEvaluatorMS {
private:
    int patch_count = 0;
    Controller* controller;
    std::string ic_path;
    std::string file_prefix;

    static uint64_t compute_median(std::vector<uint64_t> values) ;
public:
    void init(string basePath, string patchesBasePatch, SnapshotCreationStrategy* strategy, int startIndex, int endIndex, ProgressListener* progressListener = NULL);
    void init_readonly(string basePath, bool warmup = false);
    void test_lookup(string s, string p, string o, int replications, int offset, int limit);
    void cleanup_controller();
protected:
    void populate_controller_with_version(int patch_id, string path, ProgressListener* progressListener = nullptr);
    static std::ifstream::pos_type patchstore_size(Controller* controller);
    static std::ifstream::pos_type filesize(const string& file);
    static IteratorTripleString* get_from_file(const string& file);
    uint64_t measure_lookup_version_materialized(const StringTriple& triple_pattern, int offset, int patch_id, int limit, int replications, int& result_count, uint64_t& median_t);
    uint64_t measure_count_version_materialized(const StringTriple& triple_pattern, int patch_id, int replications, uint64_t& median_t);
    uint64_t measure_lookup_delta_materialized(const StringTriple& triple_pattern, int offset, int patch_id_start, int patch_id_end, int limit, int replications, int& result_count, uint64_t& median_t);
    uint64_t measure_count_delta_materialized(const StringTriple& triple_pattern, int patch_id_start, int patch_id_end, int replications, uint64_t& median_t);
    uint64_t measure_lookup_version(const StringTriple& triple_pattern, int offset, int limit, int replications, int& result_count, uint64_t& median_t);
    uint64_t measure_count_version(const StringTriple& triple_pattern, int replications, uint64_t& median_t);
};

#endif //TPFPATCH_STORE_EVALUATOR_H
