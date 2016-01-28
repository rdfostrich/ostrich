#include <iostream>
#include <string>

#include <Dictionary.hpp>
#include <HDTSpecification.hpp>
#include <HDTVocabulary.hpp>
#include <PlainDictionary.hpp>
#include <Triples.hpp>

using namespace std;
using namespace hdt;

class DictionaryManager : ModifiableDictionary {

  Dictionary *hdtDict;             // Dictionary from HDT file
  ModifiableDictionary *patchDict; // Additional dictionary

  const unsigned int bitmask;

public:
  DictionaryManager(Dictionary *hdtDict, HDTSpecification &spec)
      : hdtDict(hdtDict), bitmask(0x80000000) {
    // Create additional dictionary
    patchDict = new PlainDictionary(spec);
  };

  std::string idToString(unsigned int id, TripleComponentRole position) {
    // Check whether id is from HDT or not (MSB is not set)
    if (id & bitmask) {
      return hdtDict->idToString(id, position);
    }

    return patchDict->idToString(id, position);
  }

  unsigned int stringToId(std::string &key, TripleComponentRole position) {
    // First ask HDT
    try {
      return hdtDict->stringToId(key, position);
    } catch (exception e) {
    } // ID is not in there

    // Set MSB to 1
    return bitmask | patchDict->stringToId(key, position);
  }

  unsigned int insert(std::string &str, TripleComponentRole position) {
    // First ask HDT
    try {
      return hdtDict->stringToId(str, position);
    } catch (exception e) {
    } // ID is not in there

    return patchDict->insert(str, position);
  }
};
