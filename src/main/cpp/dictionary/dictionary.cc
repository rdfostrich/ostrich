#include <iostream>
#include <string>

#include <Dictionary.hpp>
#include <HDTSpecification.hpp>
#include <HDTVocabulary.hpp>
#include <PlainDictionary.hpp>
#include <Triples.hpp>

using namespace std;
using namespace hdt;

class DictionaryManager {

  Dictionary *hdtDict;             // Dictionary from HDT file
  ModifiableDictionary *patchDict; // Additional dictionary

  const unsigned int bitmask;

public:
  DictionaryManager(Dictionary *hdtDict)
      : hdtDict(hdtDict), bitmask(2147483648) {
    // Create additional dictionary
    patchDict = new PlainDictionary();
  };

  DictionaryManager() : bitmask(2147483648) {
    // Create two empty default dictionaries dictionary,
    hdtDict = new PlainDictionary();
    patchDict = new PlainDictionary();
  };

  ~DictionaryManager() {
    delete hdtDict;
    delete patchDict;
  }

  /**
  * Probes HDT dictionary and patch dictionary for ID and return string.
  * TODO: Uses the MSB of the id to distinguish HDT from patch. This is likely
  * to create conflicts, so needs to be altered.
  *
  * @param id The id to translate
  * @param position The position of the id in the triple
  **/
  std::string idToString(unsigned int id, TripleComponentRole position) {
    // Check whether id is from HDT or not (MSB is not set)
    if (!(id & bitmask)) {
      return hdtDict->idToString(id, position);
    }

    return patchDict->idToString(id - bitmask, position);
  }

  /**
  * Probes HDT dictionary and patch dictionary for string and return ID.
  * TODO: Uses the MSB of the id to distinguish HDT from patch. This is likely
  * to create conflicts, so needs to be altered.
  *
  * @param &str Reference to the string
  * @param position The position of the string in the triple
  **/
  unsigned int stringToId(std::string &str, TripleComponentRole position) {
    // First ask HDT
    unsigned int id;
    try {
      id = hdtDict->stringToId(str, position);
      if (id > 0)
        return id;
    } catch (exception e) {
    } // ID is not in there
    // cout << "Snapshot dictionary does not have " << str << endl;

    // Set MSB to 1
    id = patchDict->stringToId(str, position);
    return bitmask | id;
  }

  /**
  * inserts a string in the patch dictionary if it's not already in the HDT
  *
  * @param &str Reference to the string
  * @param position The position of the string in the triple
  *
  **/
  unsigned int insert(std::string &str, TripleComponentRole position) {
    // First ask HDT
    unsigned int id;
    try {
      id = hdtDict->stringToId(str, position);
      if (id > 0)
        return id;
    } catch (exception e) {
    } // ID is not in there

    // cout << "Snapshot dictionary does not have " << str << endl;

    return bitmask | patchDict->insert(str, position);
  }
};
