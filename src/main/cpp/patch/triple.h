#ifndef TPFPATCH_STORE_TRIPLE_H
#define TPFPATCH_STORE_TRIPLE_H

#include <string>

using namespace std;

// A Triple holds a subject, predicate and object
class Triple {
protected:
    string subject;
    string predicate;
    string object;
public:
    Triple();
    Triple(string subject, string predicate, string object);
    /**
     * @return The subject
     */
    string get_subject();
    /**
     * @return The predicate
     */
    string get_predicate();
    /**
     * @return The object
     */
    string get_object();
    /**
     * @return The string representation of this patch.
     */
    string to_string();
    /**
     * Serialize this value to a byte array
     * @param size This will contain the size of the returned byte array
     * @return The byte array
     */
    const char* serialize(size_t* size);
    /**
     * Deserialize the given byte array to this object.
     * @param data The data to deserialize from.
     * @param size The size of the byte array
     */
    void deserialize(const char* data, size_t size);
    bool operator < (const Triple &rhs) const {
        return subject < rhs.subject ||
                (subject == rhs.subject && (predicate < rhs.predicate ||
                        (predicate == rhs.predicate && object < rhs.object)));
    }
    bool operator == (const Triple &rhs) const { return subject == rhs.subject && predicate == rhs.predicate && object == rhs.object; }

    /**
     * Check if the given triple matches with the triple pattern.
     * @param triple The triple to check
     * @param triple_pattern The triple pattern to match with the triple, empty elements are seen as blank.
     * @return If the triple and pattern match.
     */
    static bool pattern_match_triple(Triple triple, Triple triple_pattern) {
        return (triple_pattern.get_subject() == "" || triple_pattern.get_subject() == triple.get_subject())
               && (triple_pattern.get_predicate() == "" || triple_pattern.get_predicate() == triple.get_predicate())
               && (triple_pattern.get_object() == "" || triple_pattern.get_object() == triple.get_object());
    }
    /**
     * Check if the given triple pattern has empty elements for S, P and O.
     * @param triple_pattern The triple pattern to check.
     * @return If the pattern is ? ? ?
     */
    static bool is_all_matching_pattern(Triple triple_pattern) {
        return triple_pattern.get_subject() == "" && triple_pattern.get_predicate() == "" && triple_pattern.get_object() == "";
    }
};

#endif //TPFPATCH_STORE_TRIPLE_H
