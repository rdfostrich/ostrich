#include <fstream>
#include <iostream>
#include <string>

#include <Dictionary.hpp>
#include <HDTVocabulary.hpp>
#include <Triples.hpp>
#include <dictionary/PlainDictionary.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/zlib.hpp>

#include "dictionary_manager.h"

using namespace std;
using namespace hdt;

DictionaryManager::DictionaryManager(string basePath, int snapshotId, Dictionary *hdtDict, PlainDictionary *patchDict, bool readonly)
    : basePath(basePath), snapshotId(snapshotId), hdtDict(hdtDict), patchDict(patchDict), bitmask(2147483648), readonly(readonly) {
  load();
};

DictionaryManager::DictionaryManager(string basePath, int snapshotId, Dictionary *hdtDict, bool readonly)
    : basePath(basePath), snapshotId(snapshotId), hdtDict(hdtDict), bitmask(2147483648), readonly(readonly) {
  // Create additional dictionary
  patchDict = new PlainDictionary();
  load();
};

DictionaryManager::DictionaryManager(string basePath, int snapshotId, bool readonly)
        : basePath(basePath), snapshotId(snapshotId), bitmask(2147483648), readonly(readonly) {
  // Create two empty default dictionaries dictionary,
  hdtDict = new PlainDictionary();
  patchDict = new PlainDictionary();
  load();
};

DictionaryManager::~DictionaryManager() {
  if (!readonly) {
    save();
  }
  delete patchDict;
}

void DictionaryManager::load() {
  ifstream dictFile(basePath + PATCHDICT_FILENAME_BASE(snapshotId), ios_base::in | ios_base::binary);
  if (dictFile.is_open()) {
    boost::iostreams::filtering_streambuf<boost::iostreams::input> in;
#ifdef COMPRESS_DICT
    in.push(boost::iostreams::zlib_decompressor());
#endif
    in.push(dictFile);
    in.set_auto_close(false);
    std::istream decompressed(&in);
    ControlInformation ci = ControlInformation();
    ci.load(decompressed);
    patchDict->load(decompressed, ci);
  }
}

void DictionaryManager::save() {
  ofstream dictFile;
  dictFile.open(basePath + PATCHDICT_FILENAME_BASE(snapshotId), ios_base::out | ios_base::binary);
  boost::iostreams::filtering_streambuf<boost::iostreams::output> out;
#ifdef COMPRESS_DICT
  out.push(boost::iostreams::zlib_compressor());
#endif
  out.push(dictFile);

  out.set_auto_close(false);
  std::ostream compressed(&out);
  ControlInformation ci = ControlInformation();
  patchDict->save(compressed, ci);
}

std::string DictionaryManager::idToString(unsigned int id,
                                          TripleComponentRole position) {
  unique_lock<mutex> lock(action_mutex);
  if (id == 0) return "";

  // Check whether id is from HDT or not (MSB is not set)
  if (!(id & bitmask)) {
    return hdtDict->idToString(id, position);
  }

  return patchDict->idToString(id - bitmask, position);
}

//TODO: make sure 0 is reserved for variables
unsigned int DictionaryManager::stringToId(std::string &str,
                                           TripleComponentRole position) {
  unique_lock<mutex> lock(action_mutex);
  // if string is empty, it's a variable, and thus, a zero
  if (str.empty()) return 0;

  // First ask HDT
  unsigned int id;
  try {
    id = hdtDict->stringToId(str, position);
    if (id > 0)
      return id;
  } catch (exception e) {
  } // ID is not in there
  //cout << "Snapshot dictionary does not have " << str << endl;

  // Set MSB to 1
  id = patchDict->stringToId(str, position);
  //cout << "Patch dictionary gived id  " << id << endl;
  return bitmask | id;
}

unsigned int DictionaryManager::insert(std::string &str,
                                       TripleComponentRole position) {
  unique_lock<mutex> lock(action_mutex);

  if (str.empty()) return 0;

  // First ask HDT
  unsigned int id;
  try {
    id = hdtDict->stringToId(str, position);
    //cout << "In HDT, " << str << " has id " << id << endl;
    if (id > 0)
      return id;
  } catch (exception e) {
  } // ID is not in there

  unsigned int originalId = patchDict->stringToId(str, position);
  if (originalId == 0) {
    patchDict->insert(str, position == SUBJECT ? NOT_SHARED_SUBJECT : (position == PREDICATE ? NOT_SHARED_PREDICATE
                                                                                             : NOT_SHARED_OBJECT));
    originalId = patchDict->stringToId(str, position);
  }
  id = bitmask | originalId;
  //cout << "Patch dictionary gives id  " << id << endl;
  // cout << "Snapshot dictionary does not have " << str << endl;

  return id;
}

Dictionary* DictionaryManager::getHdtDict() const {
  return hdtDict;
}

ModifiableDictionary* DictionaryManager::getPatchDict() const {
  return patchDict;
}

void DictionaryManager::cleanup(string basePath, int snapshotId) {
  std::remove((PATCHDICT_FILENAME_BASE(snapshotId)).c_str());
}

size_t DictionaryManager::getNumberOfElements() {
  return hdtDict->getNumberOfElements() + patchDict->getNumberOfElements();
}

uint64_t DictionaryManager::size() {
  return hdtDict->size() + patchDict->size();
}

unsigned int DictionaryManager::getNsubjects() {
  return hdtDict->getNsubjects() + patchDict->getNsubjects();
}

unsigned int DictionaryManager::getNpredicates() {
  return hdtDict->getNpredicates() + patchDict->getNpredicates();
}

unsigned int DictionaryManager::getNobjects() {
  return hdtDict->getNobjects() + patchDict->getNobjects();
}

unsigned int DictionaryManager::getNshared() {
  return hdtDict->getNshared() + patchDict->getNshared();
}

unsigned int DictionaryManager::getMaxID() {
  return patchDict->getMaxID();
}

unsigned int DictionaryManager::getMaxSubjectID() {
  return patchDict->getMaxSubjectID();
}

unsigned int DictionaryManager::getMaxPredicateID() {
  return patchDict->getMaxPredicateID();
}

unsigned int DictionaryManager::getMaxObjectID() {
  return patchDict->getMaxObjectID();
}

void DictionaryManager::populateHeader(Header &header, string rootNode) {}

void DictionaryManager::save(std::ostream &output, ControlInformation &ci, ProgressListener *listener) {
  patchDict->save(output, ci, listener);
}

void DictionaryManager::load(std::istream &input, ControlInformation &ci, ProgressListener *listener) {
  patchDict->load(input, ci, listener);
}

size_t DictionaryManager::load(unsigned char *ptr, unsigned char *ptrMax, ProgressListener *listener) {
  return patchDict->load(ptr, ptrMax, listener);
}

void DictionaryManager::import(Dictionary *other, ProgressListener *listener) {}

IteratorUCharString *DictionaryManager::getSubjects() {
  return new DictManagerIterator(hdtDict->getSubjects(), patchDict->getSubjects());
}

IteratorUCharString *DictionaryManager::getPredicates() {
  return new DictManagerIterator(hdtDict->getPredicates(), patchDict->getPredicates());
}

IteratorUCharString *DictionaryManager::getObjects() {
  return new DictManagerIterator(hdtDict->getObjects(), patchDict->getObjects());
}

IteratorUCharString *DictionaryManager::getShared() {
  return new DictManagerIterator(hdtDict->getShared(), patchDict->getShared());
}

void DictionaryManager::startProcessing(ProgressListener *listener) {}
void DictionaryManager::stopProcessing(ProgressListener *listener) {}

string DictionaryManager::getType() {
    return HDTVocabulary::HDT_DICTIONARY_BASE+"Manager>";
}

unsigned int DictionaryManager::getMapping() {
  return hdtDict->getMapping() + patchDict->getMapping();
}

void DictionaryManager::getSuggestions(const char *base, TripleComponentRole role, std::vector<string> &out, int maxResults) {}

int DictionaryManager::compareComponent(unsigned int componentId1, unsigned int componentId2,
                                        TripleComponentRole role) {
  unsigned int sharedCount = hdtDict->getNshared();
  bool shared1 = componentId1 > sharedCount;
  bool shared2 = componentId2 > sharedCount;
  if (shared1 == shared2) {
    return static_cast<int32_t>(componentId1 - componentId2);
  }
  return idToString(componentId1, role).compare(idToString(componentId2, role));
}
