#ifndef TPFPATCH_STORE_DICTIONARY_MANAGER_H
#define TPFPATCH_STORE_DICTIONARY_MANAGER_H

#define PATCHDICT_FILENAME_BASE(id) ("snapshotpatch_" + std::to_string(id) + ".dic")
#define COMPRESS_DICT

#include <Dictionary.hpp>
#include <dictionary/PlainDictionary.hpp>
#include <HDTVocabulary.hpp>
#include <Triples.hpp>
#include <mutex>

using namespace std;
using namespace hdt;

class DictionaryManager : public ModifiableDictionary {

    string basePath;
    Dictionary *hdtDict;             // Dictionary from HDT file
    PlainDictionary *patchDict; // Additional dictionary

    const unsigned int bitmask;
    int snapshotId;
    bool readonly;

    std::mutex action_mutex;
public:
    DictionaryManager(string basePath, int snapshotId, Dictionary *hdtDict, PlainDictionary *patchDict, bool readonly = false);
    DictionaryManager(string basePath, int snapshotId, Dictionary *hdtDict, bool readonly = false);
    DictionaryManager(string basePath, int snapshotId, bool readonly = false);
    ~DictionaryManager();

    /**
    * Probes HDT dictionary and patch dictionary for ID and return string.
    *
    * @param id The id to translate
    * @param position The position of the id in the triple
    **/
    std::string idToString(size_t id, TripleComponentRole position);

    /**
    * Probes HDT dictionary and patch dictionary for string and return ID.
    *
    * @param &str Reference to the string
    * @param position The position of the string in the triple
    **/
    size_t stringToId(const std::string &str, TripleComponentRole position);

    /**
    * inserts a string in the patch dictionary if it's not already in the HDT
    *
    * @param &str Reference to the string
    * @param position The position of the string in the triple
    *
    **/
    size_t insert(const std::string &str, TripleComponentRole position);

    Dictionary* getHdtDict() const;
    ModifiableDictionary* getPatchDict() const;
    /**
     * Removes all the files that were created by the dictionary manager of the given id.
     */
    static void cleanup(string basePath, int snapshotId);

    /**
     * @param componentId1 The first id
     * @param componentId2 The second id
     * @param role SUBJECT, PREDICATE or OBJECT
     * @return The comparisson
     */
    int compareComponent(size_t componentId1, size_t componentId2, TripleComponentRole role);

    /**
    * Proxied methods
    *
    **/

    size_t getNumberOfElements();
    uint64_t size();

    size_t getNsubjects();
    size_t getNpredicates();
    size_t getNobjects();
    size_t getNshared();
    size_t getNobjectsLiterals();
    size_t getNobjectsNotLiterals();

    size_t getMaxID();
    size_t getMaxSubjectID();
    size_t getMaxPredicateID();
    size_t getMaxObjectID();

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
    size_t getMapping();

    void getSuggestions(const char *base, TripleComponentRole role, std::vector<string> &out, int maxResults);
    hdt::IteratorUCharString *getSuggestions(const char *prefix, TripleComponentRole role);
    hdt::IteratorUInt *getIDSuggestions(const char *prefix, TripleComponentRole role);

    void save();
protected:
    void load();
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
