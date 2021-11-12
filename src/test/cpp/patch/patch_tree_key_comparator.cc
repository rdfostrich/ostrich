#include <gtest/gtest.h>

#include "../../../main/cpp/patch/patch.h"
#include "../../../main/cpp/patch/triple.h"
#include "../../../main/cpp/patch/patch_tree_key_comparator.h"
#include "../../../main/cpp/dictionary/dictionary_manager.h"
#include <dictionary/PlainDictionary.hpp>
#include <HDTEnums.hpp>
#define TESTPATH "./"

TEST(PatchTreeKeyComparatorTest, CompareSimple) {
    std::shared_ptr<DictionaryManager> dict = std::make_shared<DictionaryManager>(TESTPATH, 0);
    PatchTreeKeyComparator comp(comp_s, comp_p, comp_o, dict);
    ASSERT_EQ(-1, comp.compare(Triple("a", "a", "a", dict), Triple("b", "b", "b", dict)));
    DictionaryManager::cleanup(TESTPATH, 0);
}

TEST(PatchTreeKeyComparatorTest, CompareComplexSPO) {
    std::shared_ptr<DictionaryManager> dict = std::make_shared<DictionaryManager>(TESTPATH, 0);
    PatchTreeKeyComparator comp(comp_s, comp_p, comp_o, dict);
    ASSERT_EQ(1 , comp.compare(Triple("b", "b", "b", dict), Triple("a", "a", "a", dict)));
    ASSERT_EQ(1 , comp.compare(Triple("b", "b", "b", dict), Triple("a", "a", "b", dict)));
    ASSERT_EQ(1 , comp.compare(Triple("b", "b", "b", dict), Triple("a", "a", "c", dict)));

    ASSERT_EQ(1 , comp.compare(Triple("b", "b", "b", dict), Triple("a", "b", "a", dict)));
    ASSERT_EQ(1 , comp.compare(Triple("b", "b", "b", dict), Triple("a", "b", "b", dict)));
    ASSERT_EQ(1 , comp.compare(Triple("b", "b", "b", dict), Triple("a", "b", "c", dict)));

    ASSERT_EQ(1 , comp.compare(Triple("b", "b", "b", dict), Triple("a", "c", "a", dict)));
    ASSERT_EQ(1 , comp.compare(Triple("b", "b", "b", dict), Triple("a", "c", "b", dict)));
    ASSERT_EQ(1 , comp.compare(Triple("b", "b", "b", dict), Triple("a", "c", "c", dict)));

    ASSERT_EQ(1 , comp.compare(Triple("b", "b", "b", dict), Triple("b", "a", "a", dict)));
    ASSERT_EQ(1 , comp.compare(Triple("b", "b", "b", dict), Triple("b", "a", "b", dict)));
    ASSERT_EQ(1 , comp.compare(Triple("b", "b", "b", dict), Triple("b", "a", "c", dict)));

    ASSERT_EQ(1 , comp.compare(Triple("b", "b", "b", dict), Triple("b", "b", "a", dict)));
    ASSERT_EQ(0 , comp.compare(Triple("b", "b", "b", dict), Triple("b", "b", "b", dict)));
    ASSERT_EQ(-1, comp.compare(Triple("b", "b", "b", dict), Triple("b", "b", "c", dict)));

    ASSERT_EQ(-1, comp.compare(Triple("b", "b", "b", dict), Triple("b", "c", "a", dict)));
    ASSERT_EQ(-1, comp.compare(Triple("b", "b", "b", dict), Triple("b", "c", "b", dict)));
    ASSERT_EQ(-1, comp.compare(Triple("b", "b", "b", dict), Triple("b", "c", "c", dict)));

    ASSERT_EQ(-1, comp.compare(Triple("b", "b", "b", dict), Triple("c", "a", "a", dict)));
    ASSERT_EQ(-1, comp.compare(Triple("b", "b", "b", dict), Triple("c", "a", "b", dict)));
    ASSERT_EQ(-1, comp.compare(Triple("b", "b", "b", dict), Triple("c", "a", "c", dict)));

    ASSERT_EQ(-1, comp.compare(Triple("b", "b", "b", dict), Triple("c", "b", "a", dict)));
    ASSERT_EQ(-1, comp.compare(Triple("b", "b", "b", dict), Triple("c", "b", "b", dict)));
    ASSERT_EQ(-1, comp.compare(Triple("b", "b", "b", dict), Triple("c", "b", "c", dict)));

    ASSERT_EQ(-1, comp.compare(Triple("b", "b", "b", dict), Triple("c", "c", "a", dict)));
    ASSERT_EQ(-1, comp.compare(Triple("b", "b", "b", dict), Triple("c", "c", "b", dict)));
    ASSERT_EQ(-1, comp.compare(Triple("b", "b", "b", dict), Triple("c", "c", "c", dict)));
    DictionaryManager::cleanup(TESTPATH, 0);
}

TEST(PatchTreeKeyComparatorTest, CompareComplexSOP) {
    std::shared_ptr<DictionaryManager> dict = std::make_shared<DictionaryManager>(TESTPATH, 0);
    PatchTreeKeyComparator comp(comp_s, comp_o, comp_p, dict);
    ASSERT_EQ(1 , comp.compare(Triple("b", "b", "b", dict), Triple("a", "a", "a", dict)));
    ASSERT_EQ(1 , comp.compare(Triple("b", "b", "b", dict), Triple("a", "a", "b", dict)));
    ASSERT_EQ(1 , comp.compare(Triple("b", "b", "b", dict), Triple("a", "a", "c", dict)));

    ASSERT_EQ(1 , comp.compare(Triple("b", "b", "b", dict), Triple("a", "b", "a", dict)));
    ASSERT_EQ(1 , comp.compare(Triple("b", "b", "b", dict), Triple("a", "b", "b", dict)));
    ASSERT_EQ(1 , comp.compare(Triple("b", "b", "b", dict), Triple("a", "b", "c", dict)));

    ASSERT_EQ(1 , comp.compare(Triple("b", "b", "b", dict), Triple("a", "c", "a", dict)));
    ASSERT_EQ(1 , comp.compare(Triple("b", "b", "b", dict), Triple("a", "c", "b", dict)));
    ASSERT_EQ(1 , comp.compare(Triple("b", "b", "b", dict), Triple("a", "c", "c", dict)));

    ASSERT_EQ(1 , comp.compare(Triple("b", "b", "b", dict), Triple("b", "a", "a", dict)));
    ASSERT_EQ(1 , comp.compare(Triple("b", "b", "b", dict), Triple("b", "a", "b", dict)));
    ASSERT_EQ(-1, comp.compare(Triple("b", "b", "b", dict), Triple("b", "a", "c", dict)));

    ASSERT_EQ(1 , comp.compare(Triple("b", "b", "b", dict), Triple("b", "b", "a", dict)));
    ASSERT_EQ(0 , comp.compare(Triple("b", "b", "b", dict), Triple("b", "b", "b", dict)));
    ASSERT_EQ(-1, comp.compare(Triple("b", "b", "b", dict), Triple("b", "b", "c", dict)));

    ASSERT_EQ(1 , comp.compare(Triple("b", "b", "b", dict), Triple("b", "c", "a", dict)));
    ASSERT_EQ(-1, comp.compare(Triple("b", "b", "b", dict), Triple("b", "c", "b", dict)));
    ASSERT_EQ(-1, comp.compare(Triple("b", "b", "b", dict), Triple("b", "c", "c", dict)));

    ASSERT_EQ(-1, comp.compare(Triple("b", "b", "b", dict), Triple("c", "a", "a", dict)));
    ASSERT_EQ(-1, comp.compare(Triple("b", "b", "b", dict), Triple("c", "a", "b", dict)));
    ASSERT_EQ(-1, comp.compare(Triple("b", "b", "b", dict), Triple("c", "a", "c", dict)));

    ASSERT_EQ(-1, comp.compare(Triple("b", "b", "b", dict), Triple("c", "b", "a", dict)));
    ASSERT_EQ(-1, comp.compare(Triple("b", "b", "b", dict), Triple("c", "b", "b", dict)));
    ASSERT_EQ(-1, comp.compare(Triple("b", "b", "b", dict), Triple("c", "b", "c", dict)));

    ASSERT_EQ(-1, comp.compare(Triple("b", "b", "b", dict), Triple("c", "c", "a", dict)));
    ASSERT_EQ(-1, comp.compare(Triple("b", "b", "b", dict), Triple("c", "c", "b", dict)));
    ASSERT_EQ(-1, comp.compare(Triple("b", "b", "b", dict), Triple("c", "c", "c", dict)));
    DictionaryManager::cleanup(TESTPATH, 0);
}

TEST(PatchTreeKeyComparatorTest, CompareInterDictTypes) {
    PlainDictionary* hdtDict = new PlainDictionary();
    std::shared_ptr<DictionaryManager> dict = std::make_shared<DictionaryManager>(TESTPATH, 0, hdtDict);
    PatchTreeKeyComparator comp(comp_s, comp_o, comp_p, dict);
    string b("b");
    hdtDict->insert(b, SUBJECT);
    string d("d");
    hdtDict->insert(d, SUBJECT);
    string f("f");
    hdtDict->insert(f, SUBJECT);

    ASSERT_EQ(false, comp.compare(Triple("a", "a", "a", dict), Triple("a", "a", "a", dict)) > 0);
    ASSERT_EQ(false, comp.compare(Triple("a", "a", "a", dict), Triple("b", "a", "a", dict)) > 0);
    ASSERT_EQ(false, comp.compare(Triple("a", "a", "a", dict), Triple("c", "a", "a", dict)) > 0);
    ASSERT_EQ(false, comp.compare(Triple("a", "a", "a", dict), Triple("d", "a", "a", dict)) > 0);
    ASSERT_EQ(false, comp.compare(Triple("a", "a", "a", dict), Triple("e", "a", "a", dict)) > 0);
    ASSERT_EQ(false, comp.compare(Triple("a", "a", "a", dict), Triple("f", "a", "a", dict)) > 0);

    ASSERT_EQ(false, comp.compare(Triple("a", "a", "a", dict), Triple("a", "a", "a", dict)) < 0);
    ASSERT_EQ(false, comp.compare(Triple("b", "a", "a", dict), Triple("a", "a", "a", dict)) < 0);
    ASSERT_EQ(false, comp.compare(Triple("c", "a", "a", dict), Triple("a", "a", "a", dict)) < 0);
    ASSERT_EQ(false, comp.compare(Triple("d", "a", "a", dict), Triple("a", "a", "a", dict)) < 0);
    ASSERT_EQ(false, comp.compare(Triple("e", "a", "a", dict), Triple("a", "a", "a", dict)) < 0);
    ASSERT_EQ(false, comp.compare(Triple("f", "a", "a", dict), Triple("a", "a", "a", dict)) < 0);

    ASSERT_EQ(true , comp.compare(Triple("b", "a", "a", dict), Triple("a", "a", "a", dict)) > 0);
    ASSERT_EQ(false, comp.compare(Triple("b", "a", "a", dict), Triple("b", "a", "a", dict)) > 0);
    ASSERT_EQ(false, comp.compare(Triple("b", "a", "a", dict), Triple("c", "a", "a", dict)) > 0);
    ASSERT_EQ(false, comp.compare(Triple("b", "a", "a", dict), Triple("d", "a", "a", dict)) > 0);
    ASSERT_EQ(false, comp.compare(Triple("b", "a", "a", dict), Triple("e", "a", "a", dict)) > 0);
    ASSERT_EQ(false, comp.compare(Triple("b", "a", "a", dict), Triple("f", "a", "a", dict)) > 0);

    ASSERT_EQ(true , comp.compare(Triple("a", "a", "a", dict), Triple("b", "a", "a", dict)) < 0);
    ASSERT_EQ(false, comp.compare(Triple("b", "a", "a", dict), Triple("b", "a", "a", dict)) < 0);
    ASSERT_EQ(false, comp.compare(Triple("c", "a", "a", dict), Triple("b", "a", "a", dict)) < 0);
    ASSERT_EQ(false, comp.compare(Triple("d", "a", "a", dict), Triple("b", "a", "a", dict)) < 0);
    ASSERT_EQ(false, comp.compare(Triple("e", "a", "a", dict), Triple("b", "a", "a", dict)) < 0);
    ASSERT_EQ(false, comp.compare(Triple("f", "a", "a", dict), Triple("b", "a", "a", dict)) < 0);

    ASSERT_EQ(true , comp.compare(Triple("c", "a", "a", dict), Triple("a", "a", "a", dict)) > 0);
    ASSERT_EQ(true , comp.compare(Triple("c", "a", "a", dict), Triple("b", "a", "a", dict)) > 0);
    ASSERT_EQ(false, comp.compare(Triple("c", "a", "a", dict), Triple("c", "a", "a", dict)) > 0);
    ASSERT_EQ(false, comp.compare(Triple("c", "a", "a", dict), Triple("d", "a", "a", dict)) > 0);
    ASSERT_EQ(false, comp.compare(Triple("c", "a", "a", dict), Triple("e", "a", "a", dict)) > 0);
    ASSERT_EQ(false, comp.compare(Triple("c", "a", "a", dict), Triple("f", "a", "a", dict)) > 0);

    ASSERT_EQ(true , comp.compare(Triple("a", "a", "a", dict), Triple("c", "a", "a", dict)) < 0);
    ASSERT_EQ(true , comp.compare(Triple("b", "a", "a", dict), Triple("c", "a", "a", dict)) < 0);
    ASSERT_EQ(false, comp.compare(Triple("c", "a", "a", dict), Triple("c", "a", "a", dict)) < 0);
    ASSERT_EQ(false, comp.compare(Triple("d", "a", "a", dict), Triple("c", "a", "a", dict)) < 0);
    ASSERT_EQ(false, comp.compare(Triple("e", "a", "a", dict), Triple("c", "a", "a", dict)) < 0);
    ASSERT_EQ(false, comp.compare(Triple("f", "a", "a", dict), Triple("c", "a", "a", dict)) < 0);

    DictionaryManager::cleanup(TESTPATH, 0);
}