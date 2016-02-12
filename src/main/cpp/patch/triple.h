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
};

#endif //TPFPATCH_STORE_TRIPLE_H
