#ifndef TPFPATCH_STORE_DICTIONARY_MANAGER_H
#define TPFPATCH_STORE_DICTIONARY_MANAGER_H

#define PATCHDICT_FILENAME_BASE(id) ("snapshotpatch_" + std::to_string(id) + ".dic")

#include <Dictionary.hpp>
#include <dictionary/PlainDictionary.hpp>
#include <HDTVocabulary.hpp>
#include <Triples.hpp>

using namespace std;
using namespace hdt;

class DictionaryManager : public ModifiableDictionary {

  Dictionary *hdtDict;             // Dictionary from HDT file
  PlainDictionary *patchDict; // Additional dictionary

  const unsigned int bitmask;
  int snapshotId;

public:
  DictionaryManager(int snapshotId, Dictionary *hdtDict, PlainDictionary *patchDict);
  DictionaryManager(int snapshotId, Dictionary *hdtDict);
  DictionaryManager(int snapshotId);
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

  Dictionary* getHdtDict() const;
  ModifiableDictionary* getPatchDict() const;
  /**
   * Removes all the files that were created by the dictionary manager of the given id.
   */
  static void cleanup(int snapshotId);

  /**
  * Proxied methods
  *
  **/

  size_t getNumberOfElements();
  uint64_t size();

  unsigned int getNsubjects();
  unsigned int getNpredicates();
  unsigned int getNobjects();
  unsigned int getNshared();

  unsigned int getMaxID();
  unsigned int getMaxSubjectID();
  unsigned int getMaxPredicateID();
  unsigned int getMaxObjectID();

  void populateHeader(Header &header, string rootNode);
  void save(std::ostream &output, ControlInformation &ci, ProgressListener *listener = NULL);
  void load(std::istream &input, ControlInformation &ci, ProgressListener *listener = NULL);

  size_t load(unsigned char *ptr, unsigned char *ptrMax, ProgressListener *listener=NULL);

  void import(Dictionary *other, ProgressListener *listener=NULL);

  IteratorUCharString *getSubjects();
  IteratorUCharString *getPredicates();
  IteratorUCharString *getObjects();
  IteratorUCharString *getShared();

  void startProcessing(ProgressListener *listener = NULL);
  void stopProcessing(ProgressListener *listener = NULL);

  string getType();
  unsigned int getMapping();

  void getSuggestions(const char *base, TripleComponentRole role, std::vector<string> &out, int maxResults);
protected:
  void load();
  void save();
};

class DictManagerIterator : public IteratorUCharString {
private:
  IteratorUCharString * first;
  IteratorUCharString * second;
public:
  DictManagerIterator(IteratorUCharString *first, IteratorUCharString *second) : first(first), second(second){

  }
  virtual ~DictManagerIterator() { }

  virtual bool hasNext() {
    return first->hasNext() || first->hasNext();
  }

  virtual unsigned char *next() {
    return first->hasNext() ? first->next() : second->next();
  }
};

#endif //TPFPATCH_STORE_DICTIONARY_MANAGER_H
