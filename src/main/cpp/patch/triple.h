#ifndef TPFPATCH_STORE_TRIPLE_H
#define TPFPATCH_STORE_TRIPLE_H

#include <string>

using namespace std;

class Triple {
protected:
    string subject;
    string predicate;
    string object;
public:
    Triple();
    Triple(string subject, string predicate, string object);
    string get_subject();
    string get_predicate();
    string get_object();
    string to_string();
    const char* serialize(size_t* size);
    void deserialize(const char* data, size_t size);
    bool operator < (const Triple &rhs) const { return subject < rhs.subject && predicate < rhs.predicate && object < rhs.object; }
    bool operator == (const Triple &rhs) const { return subject == rhs.subject && predicate == rhs.predicate && object == rhs.object; }
};


#endif //TPFPATCH_STORE_TRIPLE_H
