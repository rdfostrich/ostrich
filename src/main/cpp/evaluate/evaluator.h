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
    void init(string patchesBasePatch);
    void test_lookup(string s, string p, string o);
    void cleanup_controller();
protected:
    void populate_controller_with_version(int patch_id, string path);
    IteratorTripleString* get_from_file(string file);
    long long measure_lookup(Triple triple_pattern, int offset, int patch_id, int limit);
};


#endif //TPFPATCH_STORE_EVALUATOR_H
