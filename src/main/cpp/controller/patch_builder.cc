#include "patch_builder.h"
#include "../evaluate/evaluator.h"
#include "../snapshot/vector_triple_iterator.h"

PatchBuilder::PatchBuilder(Controller* controller) : controller(controller), patch_id(-1) {
    dict = controller->get_snapshot_manager()->get_dictionary_manager(0);
    if (dict != NULL) {
        patch = new Patch(dict);
    } else {
        patch = NULL;
    }
}

PatchBuilder* PatchBuilder::set_patch_id(int patch_id) {
    this->patch_id = patch_id;
    return this;
}

void PatchBuilder::commit(ProgressListener* progressListener) {
    if (patch_id < 0) {
        patch_id = controller->get_max_patch_id() + 1;
    }
    if (patch_id == 0) {
        VectorTripleIterator* it = new VectorTripleIterator(triples);
        controller->get_snapshot_manager()->create_snapshot(0, it, BASEURI);
        delete it;
    } else {
        controller->append(*patch, patch_id, dict, progressListener);
        delete patch;
    }
}

PatchBuilder *PatchBuilder::triple(const TripleString& triple_const, bool addition) {
    if (patch != NULL) {
        TripleString& triple = const_cast<TripleString&>(triple_const);
        patch->add(PatchElement(Triple(triple.getSubject(), triple.getPredicate(), triple.getObject(), dict), addition));
    } else {
        triples.push_back(triple_const);
    }
    return this;
}

PatchBuilder* PatchBuilder::addition(const TripleString& triple) {
    return this->triple(triple, true);
}

PatchBuilder* PatchBuilder::deletion(const TripleString& triple) {
    if (patch == NULL) {
        throw std::exception(); // Impossible to add deletions in the first snapshot
    }
    return this->triple(triple, false);
}
