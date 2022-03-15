#include "patch_builder_streaming.h"
#include "controller.h"
#include "../evaluate/evaluator.h"

PatchBuilderStreaming::PatchBuilderStreaming(Controller* controller, int patch_id, bool check_uniqueness, ProgressListener* progressListener)
        : controller(controller), check_uniqueness(check_uniqueness), progressListener(progressListener) {
    int max_patch_id = controller->get_max_patch_id();
    int snapshot_id = controller->get_snapshot_manager()->get_latest_snapshot(max_patch_id);
    dict = controller->get_snapshot_manager()->get_dictionary_manager(snapshot_id);
    this->patch_id = patch_id < 0 ? max_patch_id + 1 : patch_id;
    thread = std::thread(std::bind(&PatchBuilderStreaming::threaded_insert, this));
}

void PatchBuilderStreaming::threaded_insert() {
    if (patch_id == 0) {
        controller->get_snapshot_manager()->create_snapshot(0, this, BASEURI);
    } else {
        controller->append(this, patch_id, dict, check_uniqueness, progressListener);
    }
}

void PatchBuilderStreaming::close() {
    shutdown_thread = true;
    buffer_nonempty.notify_all();
    thread.join();
}

PatchBuilderStreaming *PatchBuilderStreaming::triple(const TripleString& triple_const, bool addition) {
    if (patch_id == 0) {
        if (!addition) {
            throw std::exception(); // Impossible to add deletions in the first snapshot
        } else {
            buffer_triplestring.push_back(triple_const);
        }
    } else {
        TripleString &triple = const_cast<TripleString &>(triple_const);
        buffer_patchelements.push(PatchElement(Triple(triple.getSubject(), triple.getPredicate(), triple.getObject(), dict), addition));
    }
    return this;
}

PatchBuilderStreaming* PatchBuilderStreaming::addition(const TripleString& triple) {
    return this->triple(triple, true);
}

PatchBuilderStreaming* PatchBuilderStreaming::deletion(const TripleString& triple) {
    return this->triple(triple, false);
}

PatchBuilderStreaming::~PatchBuilderStreaming() {

}

bool PatchBuilderStreaming::next(PatchElement* element) {
    while (buffer_patchelements.size() == 0) {
        std::unique_lock<std::mutex> l(lock_thread);
        if (shutdown_thread) return false;
        buffer_nonempty.wait(l);
        if (shutdown_thread) return false;
    }
    PatchElement& buffer_element = buffer_patchelements.front();
    buffer_patchelements.pop();
    element->set_triple(buffer_element.get_triple());
    element->set_addition(buffer_element.is_addition());
    return true;
}

void PatchBuilderStreaming::goToStart() {
    if (patch_id == 0) {
        buffer_triplestring_pos = 0;
    } else {
        throw std::invalid_argument("Restarting not supported in streaming patch builder.");
    }
}

bool PatchBuilderStreaming::hasNext() {
    while (buffer_triplestring_pos >= buffer_triplestring.size()) {
        std::unique_lock<std::mutex> l(lock_thread);
        if (shutdown_thread) return false;
        buffer_nonempty.wait(l);
        if (shutdown_thread) return false;
    }
    return true;
}

TripleString* PatchBuilderStreaming::next() {
    if (buffer_triplestring_pos < buffer_triplestring.size()) {
        return new TripleString(buffer_triplestring[buffer_triplestring_pos++]);
    }
    return NULL;
}

bool PatchBuilderStreaming::hasPrevious() {
    throw std::invalid_argument("Previous not supported in streaming patch builder.");
}

TripleString *PatchBuilderStreaming::previous() {
    throw std::invalid_argument("Previous not supported in streaming patch builder.");
}

size_t PatchBuilderStreaming::getPassed() {
    return 0;
}
