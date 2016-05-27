#include <sstream>
#include <vector>
#include "triple.h"
#include "patch_tree_key_comparator.h"

// TODO: use dictionary
Triple::Triple() {}

Triple::Triple(const string& subject, const string& predicate, const string& object) :
        subject(subject), predicate(predicate), object(object) {}

const string Triple::get_subject() const {
    return subject;
}

const string Triple::get_predicate() const {
    return predicate;
}

const string Triple::get_object() const {
    return object;
}

const string Triple::to_string() const {
    return subject + " " + predicate + " " + object + ".";
}

const char* Triple::serialize(size_t* size) const {
    *size = subject.length() + predicate.length() + object.length() + 2;
    char* bytes = (char *) malloc(*size);
    memcpy(bytes, subject.c_str(), subject.length() + 1);
    memcpy(&bytes[subject.length() + 1], predicate.c_str(), predicate.length() + 1);
    memcpy(&bytes[subject.length() + predicate.length() + 2], object.c_str(), object.length());
    return bytes;
}

void Triple::deserialize(const char* data, size_t size) {
    std::vector<string*> fields(3);
    subject = "";
    predicate = "";
    object = "";
    fields[0] = &subject;
    fields[1] = &predicate;
    fields[2] = &object;
    int i = 0;
    for (int k = 0; k < size; k++) {
        if (data[k] == '\0') {
            i++;
        } else {
            *(fields[i]) += data[k];
        }
    }
}

bool Triple::operator < (const Triple& rhs) const {
    return PatchTreeKeyComparator::comparator_spo.compare(*this, rhs) < 0;
}

bool Triple::operator > (const Triple& rhs) const {
    return PatchTreeKeyComparator::comparator_spo.compare(*this, rhs) > 0;
}

bool Triple::operator == (const Triple& rhs) const {
    return PatchTreeKeyComparator::comparator_spo.compare(*this, rhs) == 0;
}

bool Triple::pattern_match_triple(const Triple& triple, const Triple& triple_pattern) {
    return (triple_pattern.get_subject() == "" || triple_pattern.get_subject() == triple.get_subject())
           && (triple_pattern.get_predicate() == "" || triple_pattern.get_predicate() == triple.get_predicate())
           && (triple_pattern.get_object() == "" || triple_pattern.get_object() == triple.get_object());
}

bool Triple::is_all_matching_pattern(const Triple& triple_pattern) {
    return triple_pattern.get_subject() == "" && triple_pattern.get_predicate() == "" && triple_pattern.get_object() == "";
}

std::size_t std::hash<Triple>::operator()(const Triple& triple) const {
    using std::size_t;
    using std::hash;
    using std::string;
    // TODO: use dict encoded values for even faster hashing!
    return ((hash<string>()(triple.get_subject())
          ^ (hash<string>()(triple.get_predicate()) << 1)) >> 1)
          ^ (hash<string>()(triple.get_object()) << 1);
}
