#include <iostream>
#include <string>

#include <Dictionary.hpp>
#include <HDTVocabulary.hpp>
#include <PlainDictionary.hpp>
#include <Triples.hpp>

#include "dictionary_manager.h"

using namespace std;
using namespace hdt;

DictionaryManager::DictionaryManager(Dictionary *hdtDict)
    : hdtDict(hdtDict), bitmask(2147483648) {
  // Create additional dictionary
  patchDict = new PlainDictionary();
};

DictionaryManager::DictionaryManager() : bitmask(2147483648) {
  // Create two empty default dictionaries dictionary,
  hdtDict = new PlainDictionary();
  patchDict = new PlainDictionary();
};

DictionaryManager::~DictionaryManager() {
  delete hdtDict;
  delete patchDict;
}

std::string DictionaryManager::idToString(unsigned int id,
                                          TripleComponentRole position) {
  // Check whether id is from HDT or not (MSB is not set)
  if (!(id & bitmask)) {
    return hdtDict->idToString(id, position);
  }

  return patchDict->idToString(id - bitmask, position);
}

unsigned int DictionaryManager::stringToId(std::string &str,
                                           TripleComponentRole position) {
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

unsigned int DictionaryManager::insert(std::string &str,
                                       TripleComponentRole position) {
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
