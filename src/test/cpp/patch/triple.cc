#include <gtest/gtest.h>

#include "../../../main/cpp/patch/patch_tree.h"
#include "../../../main/cpp/dictionary/dictionary_manager.h"
#define TESTPATH "./"

TEST(TripleTest, FieldsRaw) {
    Triple triple(11, 21, 31);
    ASSERT_EQ(11, triple.get_subject()) << "Subject is not saved correctly";
    ASSERT_EQ(21, triple.get_predicate()) << "Predicate is not saved correctly";
    ASSERT_EQ(31, triple.get_object()) << "Object is not saved correctly";
}

TEST(TripleTest, FieldsRawOverlap) {
    Triple triple(11, 11, 11);
    ASSERT_EQ(11, triple.get_subject()) << "Subject is not saved correctly";
    ASSERT_EQ(11, triple.get_predicate()) << "Predicate is not saved correctly";
    ASSERT_EQ(11, triple.get_object()) << "Object is not saved correctly";
}

TEST(TripleTest, Fields) {
    std::shared_ptr<DictionaryManager> dict = std::make_shared<DictionaryManager>(TESTPATH, 0);
    Triple triple("s1", "p1", "o1", dict);
    ASSERT_EQ("s1", triple.get_subject(*dict)) << "Subject is not saved correctly";
    ASSERT_EQ("p1", triple.get_predicate(*dict)) << "Predicate is not saved correctly";
    ASSERT_EQ("o1", triple.get_object(*dict)) << "Object is not saved correctly";
    DictionaryManager::cleanup(TESTPATH, 0);
}

TEST(TripleTest, FieldsOverlap) {
    std::shared_ptr<DictionaryManager> dict = std::make_shared<DictionaryManager>(TESTPATH, 0);
    Triple triple("a", "a", "a", dict);
    ASSERT_EQ("a", triple.get_subject(*dict)) << "Subject is not saved correctly";
    ASSERT_EQ("a", triple.get_predicate(*dict)) << "Predicate is not saved correctly";
    ASSERT_EQ("a", triple.get_object(*dict)) << "Object is not saved correctly";
    DictionaryManager::cleanup(TESTPATH, 0);
}

TEST(TripleTest, ToStringRaw) {
    Triple triple(11, 21, 31);
    ASSERT_EQ("11 21 31.", triple.to_string()) << "to_string is incorrect";
}

TEST(TripleTest, ToString) {
    std::shared_ptr<DictionaryManager> dict = std::make_shared<DictionaryManager>(TESTPATH, 0);
    Triple triple("s1", "p1", "o1", dict);
    ASSERT_EQ("s1 p1 o1.", triple.to_string(*dict)) << "to_string is incorrect";
    DictionaryManager::cleanup(TESTPATH, 0);
}

TEST(TripleTest, ToStringOverlap) {
    std::shared_ptr<DictionaryManager> dict = std::make_shared<DictionaryManager>(TESTPATH, 0);
    Triple triple("a", "a", "a", dict);
    ASSERT_EQ("a a a.", triple.to_string(*dict)) << "to_string is incorrect";
    DictionaryManager::cleanup(TESTPATH, 0);
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
    delete[] data;
}

TEST(TripleTest, Serialization) {
    std::shared_ptr<DictionaryManager> dict = std::make_shared<DictionaryManager>(TESTPATH, 0);
    Triple tripleIn("s1", "p1", "o1", dict);

    // Serialize
    size_t size;
    const char* data = tripleIn.serialize(&size);

    // Deserialize
    Triple tripleOut;
    tripleOut.deserialize(data, size);

    ASSERT_EQ(tripleIn.to_string(*dict), tripleOut.to_string(*dict)) << "Serialization failed";
    delete[] data;

    DictionaryManager::cleanup(TESTPATH, 0);
}

TEST(TripleTest, SerializationLong) {
    std::shared_ptr<DictionaryManager> dict = std::make_shared<DictionaryManager>(TESTPATH, 0);
    Triple tripleIn("abc:yioknbvfty", "def:qspdojhbgy", "ghi:pjhgfdrtyuiolk,nbvfyukl:;,n,;lkijhg", dict);

    // Serialize
    size_t size;
    const char* data = tripleIn.serialize(&size);

    // Deserialize
    Triple tripleOut;
    tripleOut.deserialize(data, size);

    ASSERT_EQ(tripleIn.to_string(*dict), tripleOut.to_string(*dict)) << "Serialization failed";
    delete[] data;

    DictionaryManager::cleanup(TESTPATH, 0);
}

#ifndef USE_VSI
TEST(TripleTest, SerializationSize) {
    std::shared_ptr<DictionaryManager> dict = std::make_shared<DictionaryManager>(TESTPATH, 0);
    Triple tripleIn("s1", "p1", "o1", dict);

    // Serialize
    size_t size;
    const char* data = tripleIn.serialize(&size);

//    ASSERT_EQ(12, size) << "Serialization length is too high";
    size_t expected_size = 3 * sizeof(size_t);
    ASSERT_EQ(expected_size, size) << "Serialization length is too high";
    delete[] data;

    DictionaryManager::cleanup(TESTPATH, 0);
}
#endif