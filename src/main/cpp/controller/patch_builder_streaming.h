#ifndef TPFPATCH_STORE_PATCH_BUILDER_STREAMING_H
#define TPFPATCH_STORE_PATCH_BUILDER_STREAMING_H

#include "../dictionary/dictionary_manager.h"
#include "../patch/patch.h"
#include <thread>
#include <mutex>
#include <condition_variable>

class Controller;

// A PatchBuilderStreaming allows the insertion of a patch element stream to a Controller using the builder pattern.
// This transparently handles both snapshot and patch creation.
// While patch insertion is fully streaming, and requires only a small amount of active memory,
// the initial snapshot requires the full stream to be materialized in-memory because of how HDT works,
// which may result in out-of-memory errors for large snapshots.
class PatchBuilderStreaming : public PatchElementIterator, hdt::IteratorTripleString {
private:
    Controller* controller;
    hdt::ProgressListener* progressListener;
    std::shared_ptr<DictionaryManager> dict;
    int patch_id;
    bool check_uniqueness;
    std::queue<PatchElement> buffer_patchelements;
    std::vector<hdt::TripleString> buffer_triplestring;
    long buffer_triplestring_pos = 0;
    std::condition_variable buffer_nonempty;
    bool shutdown_thread = false;
    std::mutex lock_thread;
    std::thread thread;
protected:
    void threaded_insert();
public:
    explicit PatchBuilderStreaming(Controller* controller, int patch_id = -1, bool check_uniqueness = false, hdt::ProgressListener* progressListener = NULL);
    ~PatchBuilderStreaming() override;
    bool next(PatchElement* element) override;
    void goToStart() override;

    bool hasNext() override;
    hdt::TripleString *next() override;
    bool hasPrevious() override;
    hdt::TripleString *previous() override;
    /**
     * Finish the stream
     * @param progressListener The progress listener
     */
    void close();
    /**
     * Add a triple.
     * @param triple The triple
     * @param addition If the triple is an addition
     * @return self
     */
    PatchBuilderStreaming* triple(const hdt::TripleString& triple, bool addition);
    /**
     * Add an addition triple.
     * @param triple The triple
     * @return self
     */
    PatchBuilderStreaming* addition(const hdt::TripleString& triple);
    /**
     * Add a deletion triple.
     * @param triple The triple
     * @return self
     */
    PatchBuilderStreaming* deletion(const hdt::TripleString& triple);

    size_t getPassed() override;
};


#endif //TPFPATCH_STORE_PATCH_BUILDER_STREAMING_H
