#include <gtest/gtest.h>

#include "../../../main/cpp/patch/patch_tree.h"

TEST(PatchElementTest, Fields) {
    PatchElement patchElement(Triple("s1", "p1", "o1"), true);
    ASSERT_EQ("s1", patchElement.get_triple().get_subject()) << "Subject is not saved correctly";
    ASSERT_EQ("p1", patchElement.get_triple().get_predicate()) << "Predicate is not saved correctly";
    ASSERT_EQ("o1", patchElement.get_triple().get_object()) << "Object is not saved correctly";
    ASSERT_EQ(true, patchElement.is_addition()) << "Addition is not saved correctly";
}

TEST(PatchElementTest, ToString) {
    PatchElement patchElement(Triple("s1", "p1", "o1"), true);
    ASSERT_EQ("s1 p1 o1. (+)", patchElement.to_string()) << "to_string is incorrect";
    PatchElement patchElement2(Triple("s2", "p2", "o2"), false);
    ASSERT_EQ("s2 p2 o2. (-)", patchElement2.to_string()) << "to_string is incorrect";
}
