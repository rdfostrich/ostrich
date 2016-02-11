#include <gtest/gtest.h>

#include "../../../main/cpp/patch/patch_tree.h"

TEST(TripleTest, Fields) {
    Triple triple("s1", "p1", "o1");
    ASSERT_EQ("s1", triple.get_subject()) << "Subject is not saved correctly";
    ASSERT_EQ("p1", triple.get_predicate()) << "Predicate is not saved correctly";
    ASSERT_EQ("o1", triple.get_object()) << "Object is not saved correctly";
}

TEST(TripleTest, ToString) {
    Triple triple("s1", "p1", "o1");
    ASSERT_EQ("s1 p1 o1.", triple.to_string()) << "to_string is incorrect";
}

TEST(TripleTest, Serialization) {
    Triple tripleIn("s1", "p1", "o1");

    // Serialize
    size_t size;
    const char* data = tripleIn.serialize(&size);

    // Deserialize
    Triple tripleOut;
    tripleOut.deserialize(data, size);

    ASSERT_EQ(tripleIn.to_string(), tripleOut.to_string()) << "Serialization failed";
    free((char*) data);
}

TEST(TripleTest, SerializationLong) {
    Triple tripleIn("abc:yioknbvfty", "def:qspdojhbgy", "ghi:pjhgfdrtyuiolk,nbvfyukl:;,n,;lkijhg");

    // Serialize
    size_t size;
    const char* data = tripleIn.serialize(&size);

    // Deserialize
    Triple tripleOut;
    tripleOut.deserialize(data, size);

    ASSERT_EQ(tripleIn.to_string(), tripleOut.to_string()) << "Serialization failed";
    free((char*) data);
}

TEST(TripleTest, SerializationSize) {
    Triple tripleIn("s1", "p1", "o1");

    // Serialize
    size_t size;
    const char* data = tripleIn.serialize(&size);

    ASSERT_EQ(8, size) << "Serialization length is too high";
    free((char*) data);
}