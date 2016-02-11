#include <gtest/gtest.h>

#include "../../../main/cpp/patch/patch.h"
#include "../../../main/cpp/patch/triple.h"
#include "../../../main/cpp/patch/patch_tree_key_comparator.h"

TEST(PatchTreeKeyComparatorTest, CompareSimple) {
    PatchTreeKeyComparator comp;
    ASSERT_EQ(-1, comp.compare(Triple("a", "a", "a"), Triple("b", "b", "b")));
}

TEST(PatchTreeKeyComparatorTest, CompareComplex) {
    PatchTreeKeyComparator comp;
    ASSERT_EQ(1 , comp.compare(Triple("b", "b", "b"), Triple("a", "a", "a")));
    ASSERT_EQ(1 , comp.compare(Triple("b", "b", "b"), Triple("a", "a", "b")));
    ASSERT_EQ(1 , comp.compare(Triple("b", "b", "b"), Triple("a", "a", "c")));

    ASSERT_EQ(1 , comp.compare(Triple("b", "b", "b"), Triple("a", "b", "a")));
    ASSERT_EQ(1 , comp.compare(Triple("b", "b", "b"), Triple("a", "b", "b")));
    ASSERT_EQ(1 , comp.compare(Triple("b", "b", "b"), Triple("a", "b", "c")));

    ASSERT_EQ(1 , comp.compare(Triple("b", "b", "b"), Triple("a", "c", "a")));
    ASSERT_EQ(1 , comp.compare(Triple("b", "b", "b"), Triple("a", "c", "b")));
    ASSERT_EQ(1 , comp.compare(Triple("b", "b", "b"), Triple("a", "c", "c")));

    ASSERT_EQ(1 , comp.compare(Triple("b", "b", "b"), Triple("b", "a", "a")));
    ASSERT_EQ(1 , comp.compare(Triple("b", "b", "b"), Triple("b", "a", "b")));
    ASSERT_EQ(1 , comp.compare(Triple("b", "b", "b"), Triple("b", "a", "c")));

    ASSERT_EQ(1 , comp.compare(Triple("b", "b", "b"), Triple("b", "b", "a")));
    ASSERT_EQ(0 , comp.compare(Triple("b", "b", "b"), Triple("b", "b", "b")));
    ASSERT_EQ(-1, comp.compare(Triple("b", "b", "b"), Triple("b", "b", "c")));

    ASSERT_EQ(-1, comp.compare(Triple("b", "b", "b"), Triple("b", "c", "a")));
    ASSERT_EQ(-1, comp.compare(Triple("b", "b", "b"), Triple("b", "c", "b")));
    ASSERT_EQ(-1, comp.compare(Triple("b", "b", "b"), Triple("b", "c", "c")));

    ASSERT_EQ(-1, comp.compare(Triple("b", "b", "b"), Triple("c", "a", "a")));
    ASSERT_EQ(-1, comp.compare(Triple("b", "b", "b"), Triple("c", "a", "b")));
    ASSERT_EQ(-1, comp.compare(Triple("b", "b", "b"), Triple("c", "a", "c")));

    ASSERT_EQ(-1, comp.compare(Triple("b", "b", "b"), Triple("c", "b", "a")));
    ASSERT_EQ(-1, comp.compare(Triple("b", "b", "b"), Triple("c", "b", "b")));
    ASSERT_EQ(-1, comp.compare(Triple("b", "b", "b"), Triple("c", "b", "c")));

    ASSERT_EQ(-1, comp.compare(Triple("b", "b", "b"), Triple("c", "c", "a")));
    ASSERT_EQ(-1, comp.compare(Triple("b", "b", "b"), Triple("c", "c", "b")));
    ASSERT_EQ(-1, comp.compare(Triple("b", "b", "b"), Triple("c", "c", "c")));
}