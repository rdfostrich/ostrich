#include <kchashdb.h>
#include <string>

#include "../patch/positioned_triple_iterator.h"
#include "../patch/triple.h"

using namespace std;
using namespace kyotocabinet;

class TriplePatternIndex {
private:
  TreeDB sp;
  TreeDB ps;

  TreeDB so;
  TreeDB os;

  TreeDB po;
  TreeDB op;

public:
  TriplePatternIndex(string file_name);
  ~TriplePatternIndex();

  PositionedTripleIterator insert(int patch_id, Triple t);

  PositionedTripleIterator find(int patch_id, Triple tp);
};
