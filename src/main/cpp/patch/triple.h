#ifndef TPFPATCH_STORE_TRIPLE_H
#define TPFPATCH_STORE_TRIPLE_H

#include <string>
#include <SingleTriple.hpp>
#include <Dictionary.hpp>

using namespace std;
using namespace hdt;

// A Triple holds a subject, predicate and object
class Triple {
protected:
  unsigned int subject;
  unsigned int predicate;
  unsigned int object;
public:
    Triple();
    Triple(const TripleID& triple);
    Triple(unsigned int subject, unsigned int predicate, unsigned int object);
    Triple(const string& s, const string& p, const string& o, ModifiableDictionary* dict);

    /**
     * @return The subject
     */
    const unsigned int get_subject() const;
    /**
     * @return The predicate
     */
    const unsigned int get_predicate() const;
    /**
     * @return The object
     */
    const unsigned int get_object() const;

    /**
     * @param dict The dictionary to decode from
     * @return The subject
     */
    const string get_subject(Dictionary& dict) const;
    /**
     * @param dict The dictionary to decode from
     * @return The predicate
     */
    const string get_predicate(Dictionary& dict) const;
    /**
     * @param dict The dictionary to decode from
     * @return The object
     */
    const string get_object(Dictionary& dict) const;

    /**
     * @return The string representation of this triple.
     */
    const string to_string() const;

    /**
     * @return The decoded string representation of this triple.
     */
    const string to_string(Dictionary& dict) const;
    /**
     * Serialize this value to a byte array
     * @param size This will contain the size of the returned byte array
     * @return The byte array
     */
    const char* serialize(size_t* size) const;
    /**
     * Deserialize the given byte array to this object.
     * @param data The data to deserialize from.
     * @param size The size of the byte array
     */
    void deserialize(const char* data, size_t size);
    void set_subject(unsigned int subject);
    void set_predicate(unsigned int predicate);
    void set_object(unsigned int object);

    bool operator == (const Triple& rhs) const;

    /**
     * Check if the given triple matches with the triple pattern.
     * @param triple The triple to check
     * @param triple_pattern The triple pattern to match with the triple, empty elements are seen as blank.
     * @return If the triple and pattern match.
     */
    static bool pattern_match_triple(const Triple& triple, const Triple& triple_pattern);
    /**
     * Check if the given triple pattern has empty elements for S, P and O.
     * @param triple_pattern The triple pattern to check.
     * @return If the pattern is ? ? ?
     */
    static bool is_all_matching_pattern(const Triple& triple_pattern);
};

namespace std {
    template <>
    struct hash<Triple> {
        std::size_t operator()(const Triple& triple) const;
    };
}

// A key in the PatchTree is a triple
typedef Triple PatchTreeKey;


#endif //TPFPATCH_STORE_TRIPLE_H
