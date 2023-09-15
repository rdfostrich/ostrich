#include "patch_builder.h"
#include "../evaluate/evaluator.h"
#include "../snapshot/vector_triple_iterator.h"


PatchBuilder::PatchBuilder(Controller* controller) : controller(controller), patch_id(-1) {
    int max_patch_id = controller->get_max_patch_id();
    int snapshot_id = controller->get_snapshot_manager()->get_latest_snapshot(max_patch_id);
    dict = controller->get_snapshot_manager()->get_dictionary_manager(snapshot_id);
    if (dict != nullptr) {
        patch = new PatchSorted(dict);
    } else {
        patch = nullptr;
    }
}

PatchBuilder* PatchBuilder::set_patch_id(int patch_id) {
    this->patch_id = patch_id;
    return this;
}

void PatchBuilder::commit(hdt::ProgressListener* progressListener) {
    if (patch_id < 0) {
        patch_id = controller->get_max_patch_id() + 1;
    }
    if (patch_id == 0) {
        VectorTripleIterator* it = new VectorTripleIterator(triples);
        controller->get_snapshot_manager()->create_snapshot(0, it, BASEURI);
        delete it;
    } else {
        patch->sort();
        controller->append(*patch, patch_id, dict, progressListener);
        delete patch;
    }
}

PatchBuilder *PatchBuilder::triple(const hdt::TripleString& triple_const, bool addition) {
    if (patch != nullptr) {
        hdt::TripleString& triple = const_cast<hdt::TripleString&>(triple_const);
        patch->add_unsorted(PatchElement(Triple(triple.getSubject(), triple.getPredicate(), triple.getObject(), dict), addition));
    } else {
        triples.push_back(triple_const);
    }
    return this;
}

PatchBuilder* PatchBuilder::addition(const hdt::TripleString& triple) {
    return this->triple(triple, true);
}

PatchBuilder* PatchBuilder::deletion(const hdt::TripleString& triple) {
    if (patch == nullptr) {
        throw std::exception(); // Impossible to add deletions in the first snapshot
    }
    return this->triple(triple, false);
}
