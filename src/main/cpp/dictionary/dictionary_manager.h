#include <Dictionary.hpp>
#include <HDTVocabulary.hpp>
#include <Triples.hpp>

using namespace std;
using namespace hdt;

class DictionaryManager {

  Dictionary *hdtDict;             // Dictionary from HDT file
  ModifiableDictionary *patchDict; // Additional dictionary

  const unsigned int bitmask;

public:
  DictionaryManager(Dictionary *hdtDict);
  DictionaryManager();
  ~DictionaryManager();

  /**
  * Probes HDT dictionary and patch dictionary for ID and return string.
  * TODO: Uses the MSB of the id to distinguish HDT from patch. This is likely
  * to create conflicts, so needs to be altered.
  *
  * @param id The id to translate
  * @param position The position of the id in the triple
  **/
  std::string idToString(unsigned int id, TripleComponentRole position);

  /**
  * Probes HDT dictionary and patch dictionary for string and return ID.
  * TODO: Uses the MSB of the id to distinguish HDT from patch. This is likely
  * to create conflicts, so needs to be altered.
  *
  * @param &str Reference to the string
  * @param position The position of the string in the triple
  **/
  unsigned int stringToId(std::string &str, TripleComponentRole position);

  /**
  * inserts a string in the patch dictionary if it's not already in the HDT
  *
  * @param &str Reference to the string
  * @param position The position of the string in the triple
  *
  **/
  unsigned int insert(std::string &str, TripleComponentRole position);
};
