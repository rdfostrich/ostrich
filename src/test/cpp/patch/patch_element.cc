#include <gtest/gtest.h>

#include "../../../main/cpp/patch/patch_tree.h"
#include "../../../main/cpp/dictionary/dictionary_manager.h"

TEST(PatchElementTest, FieldsRaw) {
    PatchElement patchElement(Triple(11, 21, 31), true);
    ASSERT_EQ(11, patchElement.get_triple().get_subject()) << "Subject is not saved correctly";
    ASSERT_EQ(21, patchElement.get_triple().get_predicate()) << "Predicate is not saved correctly";
    ASSERT_EQ(31, patchElement.get_triple().get_object()) << "Object is not saved correctly";
    ASSERT_EQ(true, patchElement.is_addition()) << "Addition is not saved correctly";
}

TEST(PatchElementTest, Fields) {
    DictionaryManager dict;
    PatchElement patchElement(Triple("s1", "p1", "o1", dict), true);
    ASSERT_EQ("s1", patchElement.get_triple().get_subject(dict)) << "Subject is not saved correctly";
    ASSERT_EQ("p1", patchElement.get_triple().get_predicate(dict)) << "Predicate is not saved correctly";
    ASSERT_EQ("o1", patchElement.get_triple().get_object(dict)) << "Object is not saved correctly";
    ASSERT_EQ(true, patchElement.is_addition()) << "Addition is not saved correctly";
}

TEST(PatchElementTest, ToString) {
    DictionaryManager dict;
    PatchElement patchElement(Triple("s1", "p1", "o1", dict), true);
    ASSERT_EQ("s1 p1 o1. (+)", patchElement.to_string(dict)) << "to_string is incorrect";
    PatchElement patchElement2(Triple("s2", "p2", "o2", dict), false);
    ASSERT_EQ("s2 p2 o2. (-)", patchElement2.to_string(dict)) << "to_string is incorrect";
}
