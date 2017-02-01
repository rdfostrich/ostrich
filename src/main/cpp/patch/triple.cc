#include <sstream>
#include <vector>
#include "triple.h"
#include "patch_tree_key_comparator.h"

Triple::Triple() : subject(0), predicate(0), object(0) {}

Triple::Triple(const TripleID& tripleId) :
        subject(tripleId.getSubject()), predicate(tripleId.getPredicate()), object(tripleId.getObject()) {}
Triple::Triple(unsigned int subject, unsigned int predicate, unsigned int object) :
        subject(subject), predicate(predicate), object(object) {}

Triple::Triple(const string& s, const string& p, const string& o, ModifiableDictionary* dict) {
  subject = !s.empty() ? dict->insert(const_cast<string&>(s), SUBJECT) : 0;
  predicate = !p.empty() ? dict->insert(const_cast<string&>(p), PREDICATE) : 0;
  object = !o.empty() ? dict->insert(const_cast<string&>(o), OBJECT) : 0;
}

const unsigned int Triple::get_subject() const {
    return subject;
}

const unsigned int Triple::get_predicate() const {
    return predicate;
}

const unsigned int Triple::get_object() const {
    return object;
}

const string Triple::get_subject(Dictionary& dict) const {
    return dict.idToString(get_subject(), SUBJECT);
}

const string Triple::get_predicate(Dictionary& dict) const {
    return dict.idToString(get_predicate(), PREDICATE);
}

const string Triple::get_object(Dictionary& dict) const {
    return dict.idToString(get_object(), OBJECT);
}

const string Triple::to_string() const {
    return std::to_string(get_subject()) + " " + std::to_string(get_predicate()) + " " + std::to_string(get_object()) + ".";
}

const string Triple::to_string(Dictionary& dict) const {
    return get_subject(dict) + " " + get_predicate(dict) + " " + get_object(dict) + ".";
}

const char* Triple::serialize(size_t* size) const {
  *size = sizeof(subject) + sizeof(predicate) + sizeof(object);
  char* bytes = (char *) malloc(*size);

  memcpy(bytes, (char*)&subject, sizeof(subject));
  memcpy(&bytes[sizeof(subject)], (char*)&predicate, sizeof(predicate));
  memcpy(&bytes[sizeof(subject) + sizeof(predicate)], (char*)&object, sizeof(object));

  return bytes;
}

void Triple::deserialize(const char* data, size_t size) {
  memcpy(&subject, data,  sizeof(subject));
  memcpy(&predicate, &data[sizeof(subject)],  sizeof(predicate));
  memcpy(&object, &data[sizeof(subject) + sizeof(predicate)],  sizeof(object));
}

void Triple::set_subject(unsigned int subject) {
    this->subject = subject;
}

void Triple::set_predicate(unsigned int predicate) {
    this->predicate = predicate;
}

void Triple::set_object(unsigned int object) {
    this->object = object;
}

bool Triple::operator == (const Triple& rhs) const {
    return get_subject() == rhs.get_subject()
           && get_predicate() == rhs.get_predicate()
           && get_object() == rhs.get_object();
}

bool Triple::pattern_match_triple(const Triple& triple, const Triple& triple_pattern) {
    return (triple_pattern.get_subject() == 0 || triple_pattern.get_subject() == triple.get_subject())
             && (triple_pattern.get_predicate() == 0 || triple_pattern.get_predicate() == triple.get_predicate())
             && (triple_pattern.get_object() == 0 || triple_pattern.get_object() == triple.get_object());
}

bool Triple::is_all_matching_pattern(const Triple& triple_pattern) {
    return triple_pattern.get_subject() == 0 && triple_pattern.get_predicate() == 0 && triple_pattern.get_object() == 0;
}

std::size_t std::hash<Triple>::operator()(const Triple& triple) const {
    using std::size_t;
    using std::hash;
    return ((triple.get_subject()
          ^ (triple.get_predicate() << 1)) >> 1)
          ^ (triple.get_object() << 1);
}
