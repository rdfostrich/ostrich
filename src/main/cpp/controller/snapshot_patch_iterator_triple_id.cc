#include <Triples.hpp>
#include "snapshot_patch_iterator_triple_id.h"
#include "../snapshot/snapshot_manager.h"

SnapshotPatchIteratorTripleID::SnapshotPatchIteratorTripleID(IteratorTripleID* snapshot_it,
                                                             PositionedTripleIterator* deletion_it,
                                                             PatchTreeKeyComparator* spo_comparator, std::shared_ptr<HDT> snapshot,
                                                             const Triple& triple_pattern, std::shared_ptr<PatchTree> patchTree,
                                                             int patch_id, int offset, PatchPosition deletion_count)
        : snapshot_it(snapshot_it), deletion_it(deletion_it), addition_it(nullptr), spo_comparator(spo_comparator),
          snapshot(snapshot), triple_pattern(triple_pattern), patchTree(patchTree), patch_id(patch_id), offset(offset),
          deletion_count(deletion_count) {
    if (deletion_it != nullptr) {
        // Reset the filter, because from here on we only need to know if a triple is in the tree or not.
        // So we don't need the filter, because this will introduce unnecessary (possibly huge for specific triple patterns) overhead.
        deletion_it->getPatchTreeIterator()->reset_triple_pattern_filter();
    }
    last_deleted_triple = new PositionedTriple();
}

SnapshotPatchIteratorTripleID::~SnapshotPatchIteratorTripleID() {
    delete last_deleted_triple;
    delete snapshot_it;
    delete deletion_it;
    delete addition_it;
}

bool SnapshotPatchIteratorTripleID::next(Triple* triple) {
    while(snapshot_it != nullptr || addition_it != nullptr) {
        if (snapshot_it != nullptr && snapshot_it->hasNext()) { // Emit triples from snapshot - deletions
            // Find snapshot triple
            TripleID* snapshot_triple = snapshot_it->next();
            triple->set_subject(snapshot_triple->getSubject());
            triple->set_predicate(snapshot_triple->getPredicate());
            triple->set_object(snapshot_triple->getObject());

            // Start iterating over deletions.
            // If we find a match, we know that we DON'T have to emit this snapshot triple.
            // If we find don't find a match and find a triple > snapshot triple, we are
            // certain that we DO have to emit this snapshot triple.
            bool emit_triple = true;
            bool found_triple_before_snapshot_triple = true;

            // Jump to the position in the tree where the snapshot triple *would be*.
            // If we find it, we skip it, because that's an actual deletion.
            // If we don't find it, emit it, because that's not a deletion.
            size_t size;
            const char* data = triple->serialize(&size);
            deletion_it->getPatchTreeIterator()->getDeletionCursor()->jump(data, size);
            deletion_it->getPatchTreeIterator()->set_triple_pattern_filter(*triple); // Only match a single triple to force early-breaking
            delete[] data;

            while (found_triple_before_snapshot_triple) {
                if (has_last_deleted_triple || deletion_it->next(last_deleted_triple, false, false)) {
                    if(last_deleted_triple->triple == *triple) {
                        // This 'confirms' the iteration step, we won't need this element hereafter.
                        has_last_deleted_triple = false;
                        emit_triple = false;
                        found_triple_before_snapshot_triple = false;
                    } else if (spo_comparator->compare(last_deleted_triple->triple, *triple) > 0) {
                        // This 'skips' the iteration step, use the same triple next iteration.
                        has_last_deleted_triple = true;
                        found_triple_before_snapshot_triple = false;
                    } else {
                        // This 'confirms' the iteration step, we won't need this element hereafter.
                        has_last_deleted_triple = false;
                    }
                } else {
                    found_triple_before_snapshot_triple = false;
                }
            }

            if(emit_triple) {
                return true;
            }
        } else { // Emit additions
            if (snapshot_it != nullptr) {
                // Calculate the offset for our addition iterator.
                long snapshot_count = snapshot_it->numResultEstimation() == EXACT ? snapshot_it->estimatedNumResults() : -1;
                if (snapshot_count == -1) {
                    snapshot_count = 0;
                    IteratorTripleID *tmp_it = SnapshotManager::search_with_offset(snapshot, triple_pattern, 0);
                    while (tmp_it->hasNext()) {
                        tmp_it->next();
                        snapshot_count++;
                    }
                    delete tmp_it;
                }

                // Delete snapshot iterator
                delete snapshot_it;
                snapshot_it = nullptr;

                // Create addition iterator with the correct offset
                long addition_offset = offset - snapshot_count + deletion_count;
                addition_it = patchTree->addition_iterator_from(addition_offset, patch_id, triple_pattern);
            }
            if(addition_it->next(triple)) {
                return true;
            } else {
                delete addition_it;
                addition_it = nullptr;
            }
        }
    }
    return false;
}
