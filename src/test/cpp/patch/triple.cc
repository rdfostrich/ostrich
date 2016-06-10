#include <gtest/gtest.h>

#include "../../../main/cpp/patch/patch_tree.h"
#include "../../../main/cpp/dictionary/dictionary_manager.h"

TEST(TripleTest, FieldsRaw) {
    Triple triple(11, 21, 31);
    ASSERT_EQ(11, triple.get_subject()) << "Subject is not saved correctly";
    ASSERT_EQ(21, triple.get_predicate()) << "Predicate is not saved correctly";
    ASSERT_EQ(31, triple.get_object()) << "Object is not saved correctly";
}

TEST(TripleTest, Fields) {
    DictionaryManager dict;
    Triple triple("s1", "p1", "o1", dict);
    ASSERT_EQ("s1", triple.get_subject(dict)) << "Subject is not saved correctly";
    ASSERT_EQ("p1", triple.get_predicate(dict)) << "Predicate is not saved correctly";
    ASSERT_EQ("o1", triple.get_object(dict)) << "Object is not saved correctly";
}

TEST(TripleTest, ToStringRaw) {
    Triple triple(11, 21, 31);
    ASSERT_EQ("11 21 31.", triple.to_string()) << "to_string is incorrect";
}

TEST(TripleTest, ToString) {
    DictionaryManager dict;
    Triple triple("s1", "p1", "o1", dict);
    ASSERT_EQ("s1 p1 o1.", triple.to_string(dict)) << "to_string is incorrect";
}

TEST(TripleTest, SerializationRaw) {
    Triple tripleIn(11, 21, 31);

    // Serialize
    size_t size;
    const char* data = tripleIn.serialize(&size);

    // Deserialize
    Triple tripleOut;
    tripleOut.deserialize(data, size);

    ASSERT_EQ(tripleIn.to_string(), tripleOut.to_string()) << "Serialization failed";
    free((char*) data);
}

TEST(TripleTest, Serialization) {
    DictionaryManager dict;
    Triple tripleIn("s1", "p1", "o1", dict);

    // Serialize
    size_t size;
    const char* data = tripleIn.serialize(&size);

    // Deserialize
    Triple tripleOut;
    tripleOut.deserialize(data, size);

    ASSERT_EQ(tripleIn.to_string(dict), tripleOut.to_string(dict)) << "Serialization failed";
    free((char*) data);
}

TEST(TripleTest, SerializationLong) {
    DictionaryManager dict;
    Triple tripleIn("abc:yioknbvfty", "def:qspdojhbgy", "ghi:pjhgfdrtyuiolk,nbvfyukl:;,n,;lkijhg", dict);

    // Serialize
    size_t size;
    const char* data = tripleIn.serialize(&size);

    // Deserialize
    Triple tripleOut;
    tripleOut.deserialize(data, size);

    ASSERT_EQ(tripleIn.to_string(dict), tripleOut.to_string(dict)) << "Serialization failed";
    free((char*) data);
}

TEST(TripleTest, SerializationSize) {
    DictionaryManager dict;
    Triple tripleIn("s1", "p1", "o1", dict);

    // Serialize
    size_t size;
    const char* data = tripleIn.serialize(&size);

    ASSERT_EQ(8, size) << "Serialization length is too high";
    free((char*) data);
}
