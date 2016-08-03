#include <gtest/gtest.h>

#include "../../../main/cpp/patch/patch.h"
#include "../../../main/cpp/patch/Triple.h"
#include "../../../main/cpp/patch/patch_tree_key_comparator.h"
#include "../../../main/cpp/dictionary/dictionary_manager.h"

TEST(PatchTreeKeyComparatorTest, CompareSimple) {
    DictionaryManager dict;
    PatchTreeKeyComparator comp(comp_s, comp_p, comp_o, &dict);
    ASSERT_EQ(-1, comp.compare(Triple("a", "a", "a", &dict), Triple("b", "b", "b", &dict)));
}

TEST(PatchTreeKeyComparatorTest, CompareComplexSPO) {
    DictionaryManager dict;
    PatchTreeKeyComparator comp(comp_s, comp_p, comp_o, &dict);
    ASSERT_EQ(1 , comp.compare(Triple("b", "b", "b", &dict), Triple("a", "a", "a", &dict)));
    ASSERT_EQ(1 , comp.compare(Triple("b", "b", "b", &dict), Triple("a", "a", "b", &dict)));
    ASSERT_EQ(1 , comp.compare(Triple("b", "b", "b", &dict), Triple("a", "a", "c", &dict)));

    ASSERT_EQ(1 , comp.compare(Triple("b", "b", "b", &dict), Triple("a", "b", "a", &dict)));
    ASSERT_EQ(1 , comp.compare(Triple("b", "b", "b", &dict), Triple("a", "b", "b", &dict)));
    ASSERT_EQ(1 , comp.compare(Triple("b", "b", "b", &dict), Triple("a", "b", "c", &dict)));

    ASSERT_EQ(1 , comp.compare(Triple("b", "b", "b", &dict), Triple("a", "c", "a", &dict)));
    ASSERT_EQ(1 , comp.compare(Triple("b", "b", "b", &dict), Triple("a", "c", "b", &dict)));
    ASSERT_EQ(1 , comp.compare(Triple("b", "b", "b", &dict), Triple("a", "c", "c", &dict)));

    ASSERT_EQ(1 , comp.compare(Triple("b", "b", "b", &dict), Triple("b", "a", "a", &dict)));
    ASSERT_EQ(1 , comp.compare(Triple("b", "b", "b", &dict), Triple("b", "a", "b", &dict)));
    ASSERT_EQ(1 , comp.compare(Triple("b", "b", "b", &dict), Triple("b", "a", "c", &dict)));

    ASSERT_EQ(1 , comp.compare(Triple("b", "b", "b", &dict), Triple("b", "b", "a", &dict)));
    ASSERT_EQ(0 , comp.compare(Triple("b", "b", "b", &dict), Triple("b", "b", "b", &dict)));
    ASSERT_EQ(-1, comp.compare(Triple("b", "b", "b", &dict), Triple("b", "b", "c", &dict)));

    ASSERT_EQ(-1, comp.compare(Triple("b", "b", "b", &dict), Triple("b", "c", "a", &dict)));
    ASSERT_EQ(-1, comp.compare(Triple("b", "b", "b", &dict), Triple("b", "c", "b", &dict)));
    ASSERT_EQ(-1, comp.compare(Triple("b", "b", "b", &dict), Triple("b", "c", "c", &dict)));

    ASSERT_EQ(-1, comp.compare(Triple("b", "b", "b", &dict), Triple("c", "a", "a", &dict)));
    ASSERT_EQ(-1, comp.compare(Triple("b", "b", "b", &dict), Triple("c", "a", "b", &dict)));
    ASSERT_EQ(-1, comp.compare(Triple("b", "b", "b", &dict), Triple("c", "a", "c", &dict)));

    ASSERT_EQ(-1, comp.compare(Triple("b", "b", "b", &dict), Triple("c", "b", "a", &dict)));
    ASSERT_EQ(-1, comp.compare(Triple("b", "b", "b", &dict), Triple("c", "b", "b", &dict)));
    ASSERT_EQ(-1, comp.compare(Triple("b", "b", "b", &dict), Triple("c", "b", "c", &dict)));

    ASSERT_EQ(-1, comp.compare(Triple("b", "b", "b", &dict), Triple("c", "c", "a", &dict)));
    ASSERT_EQ(-1, comp.compare(Triple("b", "b", "b", &dict), Triple("c", "c", "b", &dict)));
    ASSERT_EQ(-1, comp.compare(Triple("b", "b", "b", &dict), Triple("c", "c", "c", &dict)));
}

TEST(PatchTreeKeyComparatorTest, CompareComplexSOP) {
    DictionaryManager dict;
    PatchTreeKeyComparator comp(comp_s, comp_o, comp_p, &dict);
    ASSERT_EQ(1 , comp.compare(Triple("b", "b", "b", &dict), Triple("a", "a", "a", &dict)));
    ASSERT_EQ(1 , comp.compare(Triple("b", "b", "b", &dict), Triple("a", "a", "b", &dict)));
    ASSERT_EQ(1 , comp.compare(Triple("b", "b", "b", &dict), Triple("a", "a", "c", &dict)));

    ASSERT_EQ(1 , comp.compare(Triple("b", "b", "b", &dict), Triple("a", "b", "a", &dict)));
    ASSERT_EQ(1 , comp.compare(Triple("b", "b", "b", &dict), Triple("a", "b", "b", &dict)));
    ASSERT_EQ(1 , comp.compare(Triple("b", "b", "b", &dict), Triple("a", "b", "c", &dict)));

    ASSERT_EQ(1 , comp.compare(Triple("b", "b", "b", &dict), Triple("a", "c", "a", &dict)));
    ASSERT_EQ(1 , comp.compare(Triple("b", "b", "b", &dict), Triple("a", "c", "b", &dict)));
    ASSERT_EQ(1 , comp.compare(Triple("b", "b", "b", &dict), Triple("a", "c", "c", &dict)));

    ASSERT_EQ(1 , comp.compare(Triple("b", "b", "b", &dict), Triple("b", "a", "a", &dict)));
    ASSERT_EQ(1 , comp.compare(Triple("b", "b", "b", &dict), Triple("b", "a", "b", &dict)));
    ASSERT_EQ(-1, comp.compare(Triple("b", "b", "b", &dict), Triple("b", "a", "c", &dict)));

    ASSERT_EQ(1 , comp.compare(Triple("b", "b", "b", &dict), Triple("b", "b", "a", &dict)));
    ASSERT_EQ(0 , comp.compare(Triple("b", "b", "b", &dict), Triple("b", "b", "b", &dict)));
    ASSERT_EQ(-1, comp.compare(Triple("b", "b", "b", &dict), Triple("b", "b", "c", &dict)));

    ASSERT_EQ(1 , comp.compare(Triple("b", "b", "b", &dict), Triple("b", "c", "a", &dict)));
    ASSERT_EQ(-1, comp.compare(Triple("b", "b", "b", &dict), Triple("b", "c", "b", &dict)));
    ASSERT_EQ(-1, comp.compare(Triple("b", "b", "b", &dict), Triple("b", "c", "c", &dict)));

    ASSERT_EQ(-1, comp.compare(Triple("b", "b", "b", &dict), Triple("c", "a", "a", &dict)));
    ASSERT_EQ(-1, comp.compare(Triple("b", "b", "b", &dict), Triple("c", "a", "b", &dict)));
    ASSERT_EQ(-1, comp.compare(Triple("b", "b", "b", &dict), Triple("c", "a", "c", &dict)));

    ASSERT_EQ(-1, comp.compare(Triple("b", "b", "b", &dict), Triple("c", "b", "a", &dict)));
    ASSERT_EQ(-1, comp.compare(Triple("b", "b", "b", &dict), Triple("c", "b", "b", &dict)));
    ASSERT_EQ(-1, comp.compare(Triple("b", "b", "b", &dict), Triple("c", "b", "c", &dict)));

    ASSERT_EQ(-1, comp.compare(Triple("b", "b", "b", &dict), Triple("c", "c", "a", &dict)));
    ASSERT_EQ(-1, comp.compare(Triple("b", "b", "b", &dict), Triple("c", "c", "b", &dict)));
    ASSERT_EQ(-1, comp.compare(Triple("b", "b", "b", &dict), Triple("c", "c", "c", &dict)));
}