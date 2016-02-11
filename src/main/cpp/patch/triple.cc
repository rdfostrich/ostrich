#include <sstream>
#include <vector>
#include "triple.h"

// TODO: use dictionary
Triple::Triple() {}

Triple::Triple(string subject, string predicate, string object) :
        subject(subject), predicate(predicate), object(object) {}

string Triple::get_subject() {
    return subject;
}

string Triple::get_predicate() {
    return predicate;
}

string Triple::get_object() {
    return object;
}

string Triple::to_string() {
    return subject + " " + predicate + " " + object + ".";
}

const char* Triple::serialize(size_t* size) {
    *size = subject.length() + predicate.length() + object.length() + 2;
    char* bytes = (char *) malloc(*size);
    memcpy(bytes, subject.c_str(), subject.length() + 1);
    memcpy(&bytes[subject.length() + 1], predicate.c_str(), predicate.length() + 1);
    memcpy(&bytes[subject.length() + predicate.length() + 2], object.c_str(), object.length());
    return bytes;
}

void Triple::deserialize(const char* data, size_t size) {
    std::vector<string*> fields(3);
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