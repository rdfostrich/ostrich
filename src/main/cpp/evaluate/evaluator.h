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
    void test_lookup(string s, string p, string o, int replications);
    void cleanup_controller();
protected:
    void populate_controller_with_version(int patch_id, string path, ProgressListener* progressListener = NULL);
    IteratorTripleString* get_from_file(string file);
    long long measure_lookup_version_materialized(Triple triple_pattern, int offset, int patch_id, int limit, int replications);
    long long measure_lookup_delta_materialized(Triple triple_pattern, int offset, int patch_id_start, int patch_id_end, int limit, int replications);
    long long measure_lookup_version(Triple triple_pattern, int offset, int limit, int replications);
};


#endif //TPFPATCH_STORE_EVALUATOR_H
