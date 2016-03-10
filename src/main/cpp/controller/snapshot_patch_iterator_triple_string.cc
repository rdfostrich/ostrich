#include <Triples.hpp>
#include "snapshot_patch_iterator_triple_string.h"

SnapshotPatchIteratorTripleString::SnapshotPatchIteratorTripleString(
        IteratorTripleString* snapshot_it, PositionedTripleIterator* deletion_it, PatchTreeTripleIterator* addition_it)
        : snapshot_it(snapshot_it), deletion_it(deletion_it), addition_it(addition_it) {}

bool SnapshotPatchIteratorTripleString::next(Triple* triple) {
    while(snapshot_it != NULL || addition_it != NULL) {
        if (snapshot_it != NULL && snapshot_it->hasNext()) { // Emit triples from snapshot - deletions
            // Find snapshot triple
            TripleString* snapshot_triple = snapshot_it->next();

            // Start iterating over deletions.
            // If we find a match, we know that we DON'T have to emit this snapshot triple.
            // If we find don't find a match and find a triple > snapshot triple, we are
            // certain that we DO have to emit this snapshot triple.
            PositionedTriple deleted_triple;
            bool emit_triple = true;
            bool found_triple_before_snapshot_triple = true;
            while (found_triple_before_snapshot_triple) {
                if (deletion_it->next(&deleted_triple, true)) {
                    if(deleted_triple.triple == snapshot_triple) {
                        // This 'confirms' the iteration step, we won't need this element hereafter.
                        // TODO: this may be implemented more efficiently, but this will become a bit more complicated...
                        deletion_it->next(&deleted_triple);
                        emit_triple = false;
                        found_triple_before_snapshot_triple = false;
                    } else if (deleted_triple.triple > snapshot_triple) {
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
                *triple = Triple(snapshot_triple->getSubject(), snapshot_triple->getPredicate(), snapshot_triple->getObject());
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