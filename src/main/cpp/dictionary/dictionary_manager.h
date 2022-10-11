#ifndef TPFPATCH_STORE_DICTIONARY_MANAGER_H
#define TPFPATCH_STORE_DICTIONARY_MANAGER_H

#define PATCHDICT_FILENAME_BASE(id) ("snapshotpatch_" + std::to_string(id) + ".dic")
#define COMPRESS_DICT

#include <Dictionary.hpp>
#include <dictionary/PlainDictionary.hpp>
#include <HDTVocabulary.hpp>
#include <Triples.hpp>
#include <mutex>



class DictionaryManager : public hdt::ModifiableDictionary {

    std::string basePath;
    Dictionary *hdtDict;             // Dictionary from HDT file
    hdt::PlainDictionary *patchDict; // Additional dictionary

//    const size_t bitmask;
    const size_t maxHdtId;
    int snapshotId;
    bool readonly;

    std::mutex action_mutex;
public:
    DictionaryManager(std::string basePath, int snapshotId, Dictionary *hdtDict, hdt::PlainDictionary *patchDict, bool readonly = false);
    DictionaryManager(std::string basePath, int snapshotId, Dictionary *hdtDict, bool readonly = false);
    DictionaryManager(std::string basePath, int snapshotId, bool readonly = false);
    ~DictionaryManager() override;

    /**
    * Probes HDT dictionary and patch dictionary for ID and return string.
    *
    * @param id The id to translate
    * @param position The position of the id in the triple
    **/
    std::string idToString(size_t id, hdt::TripleComponentRole position) override;

    /**
    * Probes HDT dictionary and patch dictionary for string and return ID.
    *
    * @param &str Reference to the string
    * @param position The position of the string in the triple
    **/
    size_t stringToId(const std::string &str, hdt::TripleComponentRole position) override;

    /**
    * inserts a string in the patch dictionary if it's not already in the HDT
    *
    * @param &str Reference to the string
    * @param position The position of the string in the triple
    *
    **/
    size_t insert(const std::string &str, hdt::TripleComponentRole position) override;

    Dictionary* getHdtDict() const;
    ModifiableDictionary* getPatchDict() const;
    /**
     * Removes all the files that were created by the dictionary manager of the given id.
     */
    static void cleanup(std::string basePath, int snapshotId);

    /**
     * @param componentId1 The first id
     * @param componentId2 The second id
     * @param role SUBJECT, PREDICATE or OBJECT
     * @return The comparisson
     */
    int compareComponent(size_t componentId1, size_t componentId2, hdt::TripleComponentRole role);

    size_t getMaxHdtId() const;

    /**
    * Proxied methods
    *
    **/

    size_t getNumberOfElements() override;
    uint64_t size() override;

    size_t getNsubjects() override;
    size_t getNpredicates() override;
    size_t getNobjects() override;
    size_t getNshared() override;
    size_t getNobjectsLiterals() override;
    size_t getNobjectsNotLiterals() override;

    size_t getMaxID() override;
    size_t getMaxSubjectID() override;
    size_t getMaxPredicateID() override;
    size_t getMaxObjectID() override;

    void populateHeader(hdt::Header &header, string rootNode) override;
    void save(std::ostream &output, hdt::ControlInformation &ci, hdt::ProgressListener *listener = nullptr) override;
    void load(std::istream &input, hdt::ControlInformation &ci, hdt::ProgressListener *listener = nullptr) override;

    size_t load(unsigned char *ptr, unsigned char *ptrMax, hdt::ProgressListener *listener = nullptr) override;

    void import(Dictionary *other, hdt::ProgressListener *listener = nullptr) override;

    hdt::IteratorUCharString *getSubjects() override;
    hdt::IteratorUCharString *getPredicates() override;
    hdt::IteratorUCharString *getObjects() override;
    hdt::IteratorUCharString *getShared() override;

    void startProcessing(hdt::ProgressListener *listener = nullptr) override;
    void stopProcessing(hdt::ProgressListener *listener = nullptr) override;

    string getType() override;
    size_t getMapping() override;

    void getSuggestions(const char *base, hdt::TripleComponentRole role, std::vector<string> &out, int maxResults) override;
    hdt::IteratorUCharString *getSuggestions(const char *prefix, hdt::TripleComponentRole role) override;
    hdt::IteratorUInt *getIDSuggestions(const char *prefix, hdt::TripleComponentRole role) override;

    void save();
protected:
    void load();
};

class DictManagerIterator : public hdt::IteratorUCharString {
private:
    IteratorUCharString * first;
    IteratorUCharString * second;
public:
    DictManagerIterator(IteratorUCharString *first, IteratorUCharString *second) : first(first), second(second){

    }
    ~DictManagerIterator() override = default;

    bool hasNext() override {
        return first->hasNext() || first->hasNext();
    }

    unsigned char *next() override {
        return first->hasNext() ? first->next() : second->next();
    }
};

#endif //TPFPATCH_STORE_DICTIONARY_MANAGER_H
