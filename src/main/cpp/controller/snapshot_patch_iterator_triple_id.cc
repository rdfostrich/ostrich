#include <Triples.hpp>
#include "snapshot_patch_iterator_triple_id.h"

SnapshotPatchIteratorTripleID::SnapshotPatchIteratorTripleID(
        IteratorTripleID* snapshot_it, PositionedTripleIterator* deletion_it,
        PatchTreeTripleIterator* addition_it, PatchTreeKeyComparator* spo_comparator)
        : snapshot_it(snapshot_it), deletion_it(deletion_it),
          addition_it(addition_it), spo_comparator(spo_comparator) {
    if (deletion_it != NULL) {
        // Reset the filter, because from here on we only need to know if a triple is in the tree or not.
        // So we don't need the filter, because this will introduce unnecessary (possibly huge for specific triple patterns) overhead.
        deletion_it->getPatchTreeIterator()->reset_triple_pattern_filter();
    }
}

bool SnapshotPatchIteratorTripleID::next(Triple* triple) {
    while(snapshot_it != NULL || addition_it != NULL) {
        if (snapshot_it != NULL && snapshot_it->hasNext()) { // Emit triples from snapshot - deletions
            // Find snapshot triple
            TripleID* snapshot_triple = snapshot_it->next();
            *triple = Triple(snapshot_triple->getSubject(), snapshot_triple->getPredicate(), snapshot_triple->getObject());

            // Start iterating over deletions.
            // If we find a match, we know that we DON'T have to emit this snapshot triple.
            // If we find don't find a match and find a triple > snapshot triple, we are
            // certain that we DO have to emit this snapshot triple.
            PositionedTriple deleted_triple;
            bool emit_triple = true;
            bool found_triple_before_snapshot_triple = true;

            // Jump to the position in the tree where the snapshot triple *would be*.
            // If we find it, we skip it, because that's an actual deletion.
            // If we don't find it, emit it, because that's not a deletion.
            size_t size;
            const char* data = triple->serialize(&size);
            deletion_it->getPatchTreeIterator()->getDeletionCursor()->jump(data, size);
            free((void *) data);

            while (found_triple_before_snapshot_triple) {
                if (deletion_it->next(&deleted_triple, true)) {
                    if(deleted_triple.triple == *triple) {
                        // This 'confirms' the iteration step, we won't need this element hereafter.
                        // TODO: this may be implemented more efficiently, but this will become a bit more complicated...
                        deletion_it->next(&deleted_triple);
                        emit_triple = false;
                        found_triple_before_snapshot_triple = false;
                    } else if (spo_comparator->compare(deleted_triple.triple, *triple) > 0) {
                        found_triple_before_snapshot_triple = false;
                    } else {
                        // This 'confirms' the iteration step, we won't need this element hereafter.
                        // TODO: this may be implemented more efficiently, but this will become a bit more complicated...
                        deletion_it->next(&deleted_triple);
                    }
                } else {
                    found_triple_before_snapshot_triple = false;
                }
            }

            if(emit_triple) {
                return true;
            }
        } else { // Emit additions
            snapshot_it = NULL;
            *triple = Triple();
            if(addition_it->next(triple)) {
                return true;
            } else {
                addition_it = NULL;
            }
        }
    }
    return false;
}
