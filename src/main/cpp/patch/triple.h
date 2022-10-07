#ifndef TPFPATCH_STORE_TRIPLE_H
#define TPFPATCH_STORE_TRIPLE_H

#include <memory>
#include <string>
#include <SingleTriple.hpp>
#include <Dictionary.hpp>
#include "../dictionary/dictionary_manager.h"

// A Triple holds a subject, predicate and object
class Triple {
protected:
    size_t subject;
    size_t predicate;
    size_t object;
public:
    Triple();

    explicit Triple(const hdt::TripleID &triple);

    Triple(size_t subject, size_t predicate, size_t object);

    Triple(const string &s, const string &p, const string &o, std::shared_ptr<hdt::ModifiableDictionary> dict);

    /**
     * @return The subject
     */
    size_t get_subject() const;

    /**
     * @return The predicate
     */
    size_t get_predicate() const;

    /**
     * @return The object
     */
    size_t get_object() const;

    /**
     * @param dict The dictionary to decode from
     * @return The subject
     */
    string get_subject(hdt::Dictionary &dict) const;

    /**
     * @param dict The dictionary to decode from
     * @return The predicate
     */
    string get_predicate(hdt::Dictionary &dict) const;

    /**
     * @param dict The dictionary to decode from
     * @return The object
     */
    string get_object(hdt::Dictionary &dict) const;

    /**
     * @return The string representation of this triple.
     */
    string to_string() const;

    /**
     * @return The decoded string representation of this triple.
     */
    string to_string(hdt::Dictionary &dict) const;

    /**
     * Serialize this value to a byte array
     * @param size This will contain the size of the returned byte array
     * @return The byte array
     */
    const char *serialize(size_t *size) const;

    /**
     * Deserialize the given byte array to this object.
     * @param data The data to deserialize from.
     * @param size The size of the byte array
     */
    void deserialize(const char *data, size_t size);

    void set_subject(size_t subject);

    void set_predicate(size_t predicate);

    void set_object(size_t object);

    bool operator==(const Triple &rhs) const;

    /**
     * Check if the given triple matches with the triple pattern.
     * @param triple The triple to check
     * @param triple_pattern The triple pattern to match with the triple, empty elements are seen as blank.
     * @return If the triple and pattern match.
     */
    static bool pattern_match_triple(const Triple &triple, const Triple &triple_pattern);

    /**
     * Check if the given triple pattern has empty elements for S, P and O.
     * @param triple_pattern The triple pattern to check.
     * @return If the pattern is ? ? ?
     */
    static bool is_all_matching_pattern(const Triple &triple_pattern);
};

class TripleVersion {
protected:
    int patch_id;
    Triple triple;
public:
    TripleVersion(int patch_id, const Triple &triple);

    /**
     * Serialize this value to a byte array
     * @param size This will contain the size of the returned byte array
     * @return The byte array
     */
    const char *serialize(size_t *size) const;
};

namespace std {
    template<>
    struct hash<Triple> {
        std::size_t operator()(const Triple &triple) const;
    };
}

// A key in the PatchTree is a triple
typedef Triple PatchTreeKey;


class StringTriple {
private:
    std::string subject;
    std::string predicate;
    std::string object;

public:
    StringTriple();
    StringTriple(std::string s, std::string p, std::string o);

    Triple get_as_triple(std::shared_ptr<hdt::ModifiableDictionary> dict) const;

    std::string get_subject() const;
    std::string get_predicate() const;
    std::string get_object() const;
    void set_subject(std::string new_subject);
    void set_predicate(std::string new_predicate);
    void set_object(std::string new_object);

    std::string to_string() const;

    bool operator==(const StringTriple& other) const;
    bool operator<(const StringTriple& other) const;
};



// Triple annotated with versions
class TripleVersions {
protected:
    Triple* triple;
    std::vector<int>* versions;
    std::shared_ptr<DictionaryManager> dict;
public:
    TripleVersions();
    TripleVersions(Triple* triple, std::vector<int>* versions, std::shared_ptr<DictionaryManager> dictionary = nullptr);
    ~TripleVersions();
    Triple* get_triple();
    const Triple* get_triple_const() const;  // cleaner to have const pointer when we don't need to modify
    std::vector<int>* get_versions();
    std::shared_ptr<DictionaryManager> get_dictionary() const;
    void set_dictionary(std::shared_ptr<DictionaryManager> dictionary);
};

class TripleVersionsString {
protected:
    StringTriple triple;
    std::vector<int> versions;
public:
    TripleVersionsString();
    TripleVersionsString(StringTriple triple, std::vector<int> versions);
    StringTriple* get_triple();
    std::vector<int>* get_versions();

    // Compare triples strings, not versions, so we can sort a vector of TripleVersionsString
    bool operator<(const TripleVersionsString& other) const;
};



// Triple annotated with addition/deletion.
class TripleDelta {
protected:
    Triple* triple;
    bool addition;
    std::shared_ptr<DictionaryManager> dict;
public:
    TripleDelta();
    TripleDelta(Triple* triple, bool addition, std::shared_ptr<DictionaryManager> dictionary = nullptr);
    ~TripleDelta();
    Triple* get_triple();
    const Triple* get_triple_const() const;
    bool is_addition();
    void set_addition(bool addition);
    std::shared_ptr<DictionaryManager> get_dictionary() const;
    void set_dictionary(std::shared_ptr<DictionaryManager> dictionary);
};

#endif //TPFPATCH_STORE_TRIPLE_H
