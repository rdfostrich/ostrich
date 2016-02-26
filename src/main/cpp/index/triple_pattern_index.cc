#include <iostream>

#include "triple_pattern_index.h"

TriplePatternIndex::TriplePatternIndex(string file_name) {

    // Open the database
    if (!sp.open(file_name, HashDB::OWRITER | HashDB::OCREATE)) {
        cerr << "open error: " << sp.error().name() << endl;
    }
};

TriplePatternIndex::~TriplePatternIndex() {
    // Close the database
    if (!sp.close()) {
        cerr << "close error: " << sp.error().name() << endl;
    }
};


PositionedTripleIterator insert(int patch_id, Triple t) {

};

PositionedTripleIterator find(int patch_id, Triple tp) {
  if (tp.get_subject() == "" && tp.get_predicate() == "" && tp.get_object() == "") {
    DB::Cursor* cursor = db.cursor();
    cursor->jump();
    PatchTreeIterator* it = new PatchTreeIterator(cursor);
  }

};
