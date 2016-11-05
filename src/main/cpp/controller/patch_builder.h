#ifndef TPFPATCH_STORE_PATCH_BUILDER_H
#define TPFPATCH_STORE_PATCH_BUILDER_H

#include "../dictionary/dictionary_manager.h"
#include "../patch/patch.h"

class Controller;

// A PatchBuilder allows the construction of a Patch for a Controller using the builder pattern.
// This transparently handles both snapshot and patch creation.
class PatchBuilder {
private:
    Controller* controller;
    DictionaryManager* dict;
    PatchUnsorted* patch;
    std::vector<TripleString> triples;
    int patch_id;
public:
    PatchBuilder(Controller* controller);
    /**
     * Set the patch id to insert for.
     * By default, this will be last_patch_id + 1.
     * @param patch_id The new patch id
     * @return self
     */
    PatchBuilder* set_patch_id(int patch_id);
    /**
     * Insert the accumulated data from this patch into the controller.
     * @param progressListener The progress listener
     */
    void commit(ProgressListener* progressListener = NULL);
    /**
     * Add a triple.
     * @param triple The triple
     * @param addition If the triple is an addition
     * @return self
     */
    PatchBuilder* triple(const TripleString& triple, bool addition);
    /**
     * Add an addition triple.
     * @param triple The triple
     * @return self
     */
    PatchBuilder* addition(const TripleString& triple);
    /**
     * Add a deletion triple.
     * @param triple The triple
     * @return self
     */
    PatchBuilder* deletion(const TripleString& triple);
};


#endif //TPFPATCH_STORE_PATCH_BUILDER_H
