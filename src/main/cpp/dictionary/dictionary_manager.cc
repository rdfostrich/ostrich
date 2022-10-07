#include <iostream>
#include <string>

#include <Dictionary.hpp>
#include <HDTVocabulary.hpp>
#include <Triples.hpp>
#include <dictionary/PlainDictionary.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <utility>
#include <boost/iostreams/filter/zlib.hpp>

#include "dictionary_manager.h"


constexpr size_t bitmask_value = 1ULL << (8 * sizeof(size_t) - 1);

DictionaryManager::DictionaryManager(string basePath, int snapshotId, Dictionary *hdtDict, hdt::PlainDictionary *patchDict, bool readonly)
        : basePath(std::move(basePath)), snapshotId(snapshotId), hdtDict(hdtDict), patchDict(patchDict), bitmask(bitmask_value), readonly(readonly) {
    load();
};

DictionaryManager::DictionaryManager(string basePath, int snapshotId, Dictionary *hdtDict, bool readonly)
        : basePath(std::move(basePath)), snapshotId(snapshotId), hdtDict(hdtDict), bitmask(bitmask_value), readonly(readonly) {
    // Create additional dictionary
    patchDict = new hdt::PlainDictionary();
    load();
};

DictionaryManager::DictionaryManager(string basePath, int snapshotId, bool readonly)
        : basePath(std::move(basePath)), snapshotId(snapshotId), bitmask(bitmask_value), readonly(readonly) {
    // Create two empty default dictionaries dictionary,
    hdtDict = new hdt::PlainDictionary();
    patchDict = new hdt::PlainDictionary();
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
        hdt::ControlInformation ci = hdt::ControlInformation();
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
    hdt::ControlInformation ci = hdt::ControlInformation();
    patchDict->save(compressed, ci);
}

std::string DictionaryManager::idToString(size_t id,
                                          hdt::TripleComponentRole position) {
    unique_lock<mutex> lock(action_mutex);
    if (id == 0) return "";

    // Check whether id is from HDT or not (MSB is not set)
    if (!(id & bitmask)) {
        return hdtDict->idToString(id, position);
    }

    return patchDict->idToString(id - bitmask, position);
}

size_t DictionaryManager::stringToId(const std::string &str,
                                     hdt::TripleComponentRole position) {
    unique_lock<mutex> lock(action_mutex);
    // if string is empty, it's a variable, and thus, a zero
    if (str.empty()) return 0;

    // First ask HDT
    size_t id;
    try {
        id = hdtDict->stringToId(str, position);
        if (id > 0)
            return id;
    } catch (exception e) {
    } // ID is not in there

    // Set MSB to 1
    id = patchDict->stringToId(str, position);
    return bitmask | id;
}

size_t DictionaryManager::insert(const std::string &str,
                                 hdt::TripleComponentRole position) {
    unique_lock<mutex> lock(action_mutex);

    if (str.empty()) return 0;

    // First ask HDT
    size_t id;
    try {
        id = hdtDict->stringToId(str, position);
        if (id > 0)
            return id;
    } catch (exception e) {
    } // ID is not in there

    size_t originalId = patchDict->stringToId(str, position);
    if (originalId == 0) {
        patchDict->insert(str, position == hdt::SUBJECT ? hdt::NOT_SHARED_SUBJECT : (position == hdt::PREDICATE ? hdt::NOT_SHARED_PREDICATE
                                                                                                 : hdt::NOT_SHARED_OBJECT));
//        patchDict->insert(str, position);
        originalId = patchDict->stringToId(str, position);
    }
    id = bitmask | originalId;

    return id;
}

hdt::Dictionary* DictionaryManager::getHdtDict() const {
    return hdtDict;
}

hdt::ModifiableDictionary* DictionaryManager::getPatchDict() const {
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

size_t DictionaryManager::getNsubjects() {
    return hdtDict->getNsubjects() + patchDict->getNsubjects();
}

size_t DictionaryManager::getNpredicates() {
    return hdtDict->getNpredicates() + patchDict->getNpredicates();
}

size_t DictionaryManager::getNobjects() {
    return hdtDict->getNobjects() + patchDict->getNobjects();
}

size_t DictionaryManager::getNobjectsLiterals() {
    return hdtDict->getNobjectsLiterals() + patchDict->getNobjectsLiterals();
}

size_t DictionaryManager::getNobjectsNotLiterals() {
    return hdtDict->getNobjectsNotLiterals() + patchDict->getNobjectsNotLiterals();
}

size_t DictionaryManager::getNshared() {
    return hdtDict->getNshared() + patchDict->getNshared();
}

size_t DictionaryManager::getMaxID() {
    return patchDict->getMaxID();
}

size_t DictionaryManager::getMaxSubjectID() {
    return patchDict->getMaxSubjectID();
}

size_t DictionaryManager::getMaxPredicateID() {
    return patchDict->getMaxPredicateID();
}

size_t DictionaryManager::getMaxObjectID() {
    return patchDict->getMaxObjectID();
}

void DictionaryManager::populateHeader(hdt::Header &header, string rootNode) {}

void DictionaryManager::save(std::ostream &output, hdt::ControlInformation &ci, hdt::ProgressListener *listener) {
    patchDict->save(output, ci, listener);
}

void DictionaryManager::load(std::istream &input, hdt::ControlInformation &ci, hdt::ProgressListener *listener) {
    patchDict->load(input, ci, listener);
}

size_t DictionaryManager::load(unsigned char *ptr, unsigned char *ptrMax, hdt::ProgressListener *listener) {
    return patchDict->load(ptr, ptrMax, listener);
}

void DictionaryManager::import(Dictionary *other, hdt::ProgressListener *listener) {}

hdt::IteratorUCharString *DictionaryManager::getSubjects() {
    return new DictManagerIterator(hdtDict->getSubjects(), patchDict->getSubjects());
}

hdt::IteratorUCharString *DictionaryManager::getPredicates() {
    return new DictManagerIterator(hdtDict->getPredicates(), patchDict->getPredicates());
}

hdt::IteratorUCharString *DictionaryManager::getObjects() {
    return new DictManagerIterator(hdtDict->getObjects(), patchDict->getObjects());
}

hdt::IteratorUCharString *DictionaryManager::getShared() {
    return new DictManagerIterator(hdtDict->getShared(), patchDict->getShared());
}

void DictionaryManager::startProcessing(hdt::ProgressListener *listener) {}
void DictionaryManager::stopProcessing(hdt::ProgressListener *listener) {}

string DictionaryManager::getType() {
    return hdt::HDTVocabulary::HDT_DICTIONARY_BASE+"Manager>";
}

size_t DictionaryManager::getMapping() {
    return hdtDict->getMapping() + patchDict->getMapping();
}

void DictionaryManager::getSuggestions(const char *base, hdt::TripleComponentRole role, std::vector<string> &out, int maxResults) {}

hdt::IteratorUCharString* DictionaryManager::getSuggestions(const char *prefix, hdt::TripleComponentRole role) {
    return nullptr;
}

hdt::IteratorUInt* DictionaryManager::getIDSuggestions(const char *prefix, hdt::TripleComponentRole role) {
    return nullptr;
}

int DictionaryManager::compareComponent(size_t componentId1, size_t componentId2,
                                        hdt::TripleComponentRole role) {
    size_t sharedCount = hdtDict->getNshared();
    bool shared1 = componentId1 > sharedCount;
    bool shared2 = componentId2 > sharedCount;
    if (shared1 == shared2) {
        return static_cast<int>(componentId1 - componentId2);
    }
    return idToString(componentId1, role).compare(idToString(componentId2, role));
}
