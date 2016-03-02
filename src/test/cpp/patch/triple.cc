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

TEST(TripleTest, Compare1) {
    ASSERT_EQ(true, Triple("s1", "p1", "o1") < Triple("s2", "p1", "o1")) << "Comparisson is incorrect";
    ASSERT_EQ(false, Triple("s1", "p1", "o1") == Triple("s2", "p1", "o1")) << "Comparisson is incorrect";

    ASSERT_EQ(false, Triple("s", "p", "o") < Triple("s", "p", "o")) << "Comparisson is incorrect";
    ASSERT_EQ(true, Triple("s", "p", "o") == Triple("s", "p", "o")) << "Comparisson is incorrect";
}

TEST(TripleTest, Compare2) {
    ASSERT_EQ(false, Triple("b", "b", "b") < Triple("a", "a", "a"));
    ASSERT_EQ(false, Triple("b", "b", "b") < Triple("a", "a", "b"));
    ASSERT_EQ(false, Triple("b", "b", "b") < Triple("a", "a", "c"));

    ASSERT_EQ(false, Triple("b", "b", "b") < Triple("a", "b", "a"));
    ASSERT_EQ(false, Triple("b", "b", "b") < Triple("a", "b", "b"));
    ASSERT_EQ(false, Triple("b", "b", "b") < Triple("a", "b", "c"));

    ASSERT_EQ(false, Triple("b", "b", "b") < Triple("a", "c", "a"));
    ASSERT_EQ(false, Triple("b", "b", "b") < Triple("a", "c", "b"));
    ASSERT_EQ(false, Triple("b", "b", "b") < Triple("a", "c", "c"));

    ASSERT_EQ(false, Triple("b", "b", "b") < Triple("b", "a", "a"));
    ASSERT_EQ(false, Triple("b", "b", "b") < Triple("b", "a", "b"));
    ASSERT_EQ(false, Triple("b", "b", "b") < Triple("b", "a", "c"));

    ASSERT_EQ(false, Triple("b", "b", "b") < Triple("b", "b", "a"));
    ASSERT_EQ(false, Triple("b", "b", "b") < Triple("b", "b", "b"));
    ASSERT_EQ(true , Triple("b", "b", "b") < Triple("b", "b", "c"));

    ASSERT_EQ(true , Triple("b", "b", "b") < Triple("b", "c", "a"));
    ASSERT_EQ(true , Triple("b", "b", "b") < Triple("b", "c", "b"));
    ASSERT_EQ(true , Triple("b", "b", "b") < Triple("b", "c", "c"));

    ASSERT_EQ(true , Triple("b", "b", "b") < Triple("c", "a", "a"));
    ASSERT_EQ(true , Triple("b", "b", "b") < Triple("c", "a", "b"));
    ASSERT_EQ(true , Triple("b", "b", "b") < Triple("c", "a", "c"));

    ASSERT_EQ(true , Triple("b", "b", "b") < Triple("c", "b", "a"));
    ASSERT_EQ(true , Triple("b", "b", "b") < Triple("c", "b", "b"));
    ASSERT_EQ(true , Triple("b", "b", "b") < Triple("c", "b", "c"));

    ASSERT_EQ(true , Triple("b", "b", "b") < Triple("c", "c", "a"));
    ASSERT_EQ(true , Triple("b", "b", "b") < Triple("c", "c", "b"));
    ASSERT_EQ(true , Triple("b", "b", "b") < Triple("c", "c", "c"));
}