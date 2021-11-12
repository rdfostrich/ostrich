#include <gtest/gtest.h>

#include "../../../main/cpp/patch/patch_tree.h"
#include "../../../main/cpp/patch/patch_tree_deletion_value.h"
#include "../../../main/cpp/dictionary/dictionary_manager.h"
#define TESTPATH "./"

// The fixture for testing class Patch.
class PatchElementsTest : public ::testing::Test {
protected:
    std::shared_ptr<DictionaryManager> dict;
    PatchSorted patchElements;

    PatchElementsTest() : dict(std::make_shared<DictionaryManager>(TESTPATH, 0)), patchElements(PatchSorted(dict)) {}

    virtual void SetUp() {
        patchElements = PatchSorted(dict);
    }

    virtual void TearDown() {
        DictionaryManager::cleanup(TESTPATH, 0);
    }
};

TEST_F(PatchElementsTest, AddSingle) {
    patchElements.add(PatchElement(Triple("s1", "p1", "o1", dict), true));
}

TEST_F(PatchElementsTest, AddMultiple) {
    patchElements.add(PatchElement(Triple("s1", "p1", "o1", dict), true));
    patchElements.add(PatchElement(Triple("s2", "p2", "o2", dict), false));
    patchElements.add(PatchElement(Triple("s3", "p3", "o3", dict), false));
    patchElements.add(PatchElement(Triple("s4", "p4", "o4", dict), true));
}

TEST_F(PatchElementsTest, AddAll) {
    PatchSorted patch2(dict);

    patchElements.add(PatchElement(Triple("s1", "p1", "o1", dict), true));
    patchElements.add(PatchElement(Triple("s3", "p3", "o3", dict), false));

    patch2.add(PatchElement(Triple("s2", "p2", "o2", dict), false));
    patch2.add(PatchElement(Triple("s4", "p4", "o4", dict), true));
    patchElements.addAll(patch2);

    ASSERT_EQ("s1 p1 o1. (+)", patchElements.get(0).to_string(*dict)) << "First element is incorrect";
    ASSERT_EQ("s2 p2 o2. (-)", patchElements.get(1).to_string(*dict)) << "Second element is incorrect";
    ASSERT_EQ("s3 p3 o3. (-)", patchElements.get(2).to_string(*dict)) << "Third element is incorrect";
    ASSERT_EQ("s4 p4 o4. (+)", patchElements.get(3).to_string(*dict)) << "Fourth element is incorrect";
}

TEST_F(PatchElementsTest, AddAllOverlap) {
    PatchSorted patch2(dict);

    patchElements.add(PatchElement(Triple("s1", "p1", "o1", dict), true));
    patchElements.add(PatchElement(Triple("s3", "p3", "o3", dict), false));

    patch2.add(PatchElement(Triple("s2", "p2", "o2", dict), false));
    patch2.add(PatchElement(Triple("s1", "p1", "o1", dict), false));
    patchElements.addAll(patch2);

    ASSERT_EQ("s1 p1 o1. (-)", patchElements.get(0).to_string(*dict)) << "First element is incorrect";
    ASSERT_EQ("s2 p2 o2. (-)", patchElements.get(1).to_string(*dict)) << "Third element is incorrect";
    ASSERT_EQ("s3 p3 o3. (-)", patchElements.get(2).to_string(*dict)) << "Fourth element is incorrect";
}

TEST_F(PatchElementsTest, ToString) {
    patchElements.add(PatchElement(Triple("s1", "p1", "o1", dict), true));
    patchElements.add(PatchElement(Triple("s2", "p2", "o2", dict), false));
    patchElements.add(PatchElement(Triple("s3", "p3", "o3", dict), false));
    patchElements.add(PatchElement(Triple("s4", "p4", "o4", dict), true));

    ASSERT_EQ("s1 p1 o1. (+)\n"
              "s2 p2 o2. (-)\n"
              "s3 p3 o3. (-)\n"
              "s4 p4 o4. (+)\n", patchElements.to_string(*dict)) << "to_string(dict) is invalid";
}

TEST_F(PatchElementsTest, GetSize) {
    ASSERT_EQ(0, patchElements.get_size()) << "Size of empty patch must be 0";

    patchElements.add(PatchElement(Triple("s1", "p1", "o1", dict), true));
    ASSERT_EQ(1, patchElements.get_size()) << "Size of patch with one element must be 1";

    patchElements.add(PatchElement(Triple("s2", "p2", "o2", dict), false));
    patchElements.add(PatchElement(Triple("s3", "p3", "o3", dict), false));
    patchElements.add(PatchElement(Triple("s4", "p4", "o4", dict), true));
    ASSERT_EQ(4, patchElements.get_size()) << "Size of patch with four elements must be 4";
}

TEST_F(PatchElementsTest, GetInvalidEmpty) {
    try {
        patchElements.get(0);
        FAIL() << "Getting by out of bound index must be NULL";
    } catch (const std::invalid_argument& err) {
        ASSERT_STREQ("Index out of bounds", err.what());
    }
    try {
        patchElements.get(-1);
        FAIL() << "Getting by out of bound index must be NULL";
    } catch (const std::invalid_argument& err) {
        ASSERT_STREQ("Index out of bounds", err.what());
    }
    try {
        patchElements.get(1);
        FAIL() << "Getting by out of bound index must be NULL";
    } catch (const std::invalid_argument& err) {
        ASSERT_STREQ("Index out of bounds", err.what());
    }
}

TEST_F(PatchElementsTest, GetInvalidNonEmpty) {
    patchElements.add(PatchElement(Triple("s1", "p1", "o1", dict), true));
    patchElements.get(0);
    try {
        patchElements.get(1);
        FAIL() << "Getting by out of bound index must be NULL";
    } catch (const std::invalid_argument& err) {
        ASSERT_STREQ("Index out of bounds", err.what());
    }
    try {
        patchElements.get(-1);
        FAIL() << "Getting by out of bound index must be NULL";
    } catch (const std::invalid_argument& err) {
        ASSERT_STREQ("Index out of bounds", err.what());
    }
}

TEST_F(PatchElementsTest, GetSingle) {
    PatchElement patchElement1(Triple("s1", "p1", "o1", dict), true);
    patchElements.add(patchElement1);
    PatchElement patchElement1Got = patchElements.get(0);

    ASSERT_EQ(patchElement1.is_addition(), patchElement1Got.is_addition()) << "Retrieved patch type does not equal the inserted patch type";
    ASSERT_EQ(patchElement1.get_triple().get_subject(*dict), patchElement1Got.get_triple().get_subject(*dict)) << "Retrieved patch subject does not equal the inserted patch subject";
    ASSERT_EQ(patchElement1.get_triple().get_predicate(*dict), patchElement1Got.get_triple().get_predicate(*dict)) << "Retrieved patch predicate does not equal the inserted patch predicate";
    ASSERT_EQ(patchElement1.get_triple().get_object(*dict), patchElement1Got.get_triple().get_object(*dict)) << "Retrieved patch object does not equal the inserted patch object";
}

TEST_F(PatchElementsTest, GetMultiple) {
    PatchElement patchElement1(Triple("s1", "p1", "o1", dict), true);
    PatchElement patchElement2(Triple("s2", "p2", "o2", dict), false);
    PatchElement patchElement3(Triple("s3", "p3", "o3", dict), false);
    PatchElement patchElement4(Triple("s4", "p4", "o4", dict), true);
    patchElements.add(patchElement1);
    patchElements.add(patchElement2);
    patchElements.add(patchElement3);
    patchElements.add(patchElement4);
    PatchElement patchElement1Got = patchElements.get(0);
    PatchElement patchElement2Got = patchElements.get(1);
    PatchElement patchElement3Got = patchElements.get(2);
    PatchElement patchElement4Got = patchElements.get(3);

    ASSERT_EQ(patchElement1.is_addition(), patchElement1Got.is_addition()) << "Retrieved patch 1 type does not equal the inserted patch type";
    ASSERT_EQ(patchElement1.get_triple().get_subject(*dict), patchElement1Got.get_triple().get_subject(*dict)) << "Retrieved patch 1 subject does not equal the inserted patch subject";
    ASSERT_EQ(patchElement1.get_triple().get_predicate(*dict), patchElement1Got.get_triple().get_predicate(*dict)) << "Retrieved patch 1 predicate does not equal the inserted patch predicate";
    ASSERT_EQ(patchElement1.get_triple().get_object(*dict), patchElement1Got.get_triple().get_object(*dict)) << "Retrieved patch 1 object does not equal the inserted patch object";

    ASSERT_EQ(patchElement2.is_addition(), patchElement2Got.is_addition()) << "Retrieved patch 2 type does not equal the inserted patch type";
    ASSERT_EQ(patchElement2.get_triple().get_subject(*dict), patchElement2Got.get_triple().get_subject(*dict)) << "Retrieved patch 2 subject does not equal the inserted patch subject";
    ASSERT_EQ(patchElement2.get_triple().get_predicate(*dict), patchElement2Got.get_triple().get_predicate(*dict)) << "Retrieved patch 2 predicate does not equal the inserted patch predicate";
    ASSERT_EQ(patchElement2.get_triple().get_object(*dict), patchElement2Got.get_triple().get_object(*dict)) << "Retrieved patch 2 object does not equal the inserted patch object";

    ASSERT_EQ(patchElement3.is_addition(), patchElement3Got.is_addition()) << "Retrieved patch 3 type does not equal the inserted patch type";
    ASSERT_EQ(patchElement3.get_triple().get_subject(*dict), patchElement3Got.get_triple().get_subject(*dict)) << "Retrieved patch 3 subject does not equal the inserted patch subject";
    ASSERT_EQ(patchElement3.get_triple().get_predicate(*dict), patchElement3Got.get_triple().get_predicate(*dict)) << "Retrieved patch 3 predicate does not equal the inserted patch predicate";
    ASSERT_EQ(patchElement3.get_triple().get_object(*dict), patchElement3Got.get_triple().get_object(*dict)) << "Retrieved patch 3 object does not equal the inserted patch object";

    ASSERT_EQ(patchElement4.is_addition(), patchElement4Got.is_addition()) << "Retrieved patch 4 type does not equal the inserted patch type";
    ASSERT_EQ(patchElement4.get_triple().get_subject(*dict), patchElement4Got.get_triple().get_subject(*dict)) << "Retrieved patch 4 subject does not equal the inserted patch subject";
    ASSERT_EQ(patchElement4.get_triple().get_predicate(*dict), patchElement4Got.get_triple().get_predicate(*dict)) << "Retrieved patch 4 predicate does not equal the inserted patch predicate";
    ASSERT_EQ(patchElement4.get_triple().get_object(*dict), patchElement4Got.get_triple().get_object(*dict)) << "Retrieved patch 4 object does not equal the inserted patch object";
}

TEST_F(PatchElementsTest, Order1) {
    patchElements.add(PatchElement(Triple("s3", "p3", "o3", dict), false));
    patchElements.add(PatchElement(Triple("s1", "p1", "o1", dict), false));
    patchElements.add(PatchElement(Triple("s4", "p4", "o4", dict), true));
    patchElements.add(PatchElement(Triple("s2", "p2", "o2", dict), true));

    ASSERT_EQ("s1 p1 o1. (-)", patchElements.get(0).to_string(*dict)) << "First element is incorrect";
    ASSERT_EQ("s2 p2 o2. (+)", patchElements.get(1).to_string(*dict)) << "Second element is incorrect";
    ASSERT_EQ("s3 p3 o3. (-)", patchElements.get(2).to_string(*dict)) << "Third element is incorrect";
    ASSERT_EQ("s4 p4 o4. (+)", patchElements.get(3).to_string(*dict)) << "Fourth element is incorrect";
}

TEST_F(PatchElementsTest, Order2) {
    patchElements.add(PatchElement(Triple("s4", "p4", "o4", dict), true));
    patchElements.add(PatchElement(Triple("s3", "p3", "o3", dict), false));
    patchElements.add(PatchElement(Triple("s2", "p2", "o2", dict), true));
    patchElements.add(PatchElement(Triple("s1", "p1", "o1", dict), false));

    ASSERT_EQ("s1 p1 o1. (-)", patchElements.get(0).to_string(*dict)) << "First element is incorrect";
    ASSERT_EQ("s2 p2 o2. (+)", patchElements.get(1).to_string(*dict)) << "Second element is incorrect";
    ASSERT_EQ("s3 p3 o3. (-)", patchElements.get(2).to_string(*dict)) << "Third element is incorrect";
    ASSERT_EQ("s4 p4 o4. (+)", patchElements.get(3).to_string(*dict)) << "Fourth element is incorrect";
}

TEST_F(PatchElementsTest, Order3) {
    patchElements.add(PatchElement(Triple("q", "p", "o", dict), false));
    patchElements.add(PatchElement(Triple("g", "p", "o", dict), true));
    patchElements.add(PatchElement(Triple("s", "z", "o", dict), false));
    patchElements.add(PatchElement(Triple("s", "a", "o", dict), true));

    ASSERT_EQ("g p o. (+)", patchElements.get(0).to_string(*dict)) << "First element is incorrect";
    ASSERT_EQ("q p o. (-)", patchElements.get(1).to_string(*dict)) << "Second element is incorrect";
    ASSERT_EQ("s a o. (+)", patchElements.get(2).to_string(*dict)) << "Third element is incorrect";
    ASSERT_EQ("s z o. (-)", patchElements.get(3).to_string(*dict)) << "Fourth element is incorrect";
}

TEST_F(PatchElementsTest, Order4) {
    patchElements.add(PatchElement(Triple("s", "z", "o", dict), false));
    patchElements.add(PatchElement(Triple("g", "p", "o", dict), true));
    patchElements.add(PatchElement(Triple("q", "p", "o", dict), false));
    patchElements.add(PatchElement(Triple("s", "a", "o", dict), true));

    ASSERT_EQ("g p o. (+)", patchElements.get(0).to_string(*dict)) << "First element is incorrect";
    ASSERT_EQ("q p o. (-)", patchElements.get(1).to_string(*dict)) << "Second element is incorrect";
    ASSERT_EQ("s a o. (+)", patchElements.get(2).to_string(*dict)) << "Third element is incorrect";
    ASSERT_EQ("s z o. (-)", patchElements.get(3).to_string(*dict)) << "Fourth element is incorrect";
}

TEST_F(PatchElementsTest, PositionPresent) {
    patchElements.add(PatchElement(Triple("s", "z", "o", dict), false));
    patchElements.add(PatchElement(Triple("g", "p", "o", dict), true));
    patchElements.add(PatchElement(Triple("q", "p", "o", dict), false));
    patchElements.add(PatchElement(Triple("s", "a", "o", dict), true));

    ASSERT_EQ(0, patchElements.position_of(PatchElement(Triple("g", "p", "o", dict), true ))) << "Found position is wrong";
    ASSERT_EQ(1, patchElements.position_of(PatchElement(Triple("q", "p", "o", dict), false))) << "Found position is wrong";
    ASSERT_EQ(2, patchElements.position_of(PatchElement(Triple("s", "a", "o", dict), true ))) << "Found position is wrong";
    ASSERT_EQ(3, patchElements.position_of(PatchElement(Triple("s", "z", "o", dict), false))) << "Found position is wrong";
}

TEST_F(PatchElementsTest, Positions) {
    PatchElement e_0 = PatchElement(Triple("s", "z", "o", dict), false);
    PatchElement e_1 = PatchElement(Triple("g", "p", "o", dict), true);
    PatchElement e_2 = PatchElement(Triple("q", "p", "o", dict), false);
    PatchElement e_3 = PatchElement(Triple("s", "a", "o", dict), true);

    patchElements.add(e_0);
    patchElements.add(e_1);
    patchElements.add(e_2);
    patchElements.add(e_3);
    // Expected order:
    // g p o +
    // q p o -
    // s a o +
    // s z o -

    // Calculate positions
    HashDB sp_;
    HashDB s_o;
    HashDB s__;
    HashDB _po;
    HashDB _p_;
    HashDB __o;
    sp_.open(".additions.sp_.tmp", HashDB::OWRITER | HashDB::OCREATE);
    s_o.open(".additions.s_o.tmp", HashDB::OWRITER | HashDB::OCREATE);
    s__.open(".additions.s__.tmp", HashDB::OWRITER | HashDB::OCREATE);
    _po.open(".additions._po.tmp", HashDB::OWRITER | HashDB::OCREATE);
    _p_.open(".additions._p_.tmp", HashDB::OWRITER | HashDB::OCREATE);
    __o.open(".additions.__o.tmp", HashDB::OWRITER | HashDB::OCREATE);
    PatchPosition ___ = 0;

    // Simulate patch-position calculation
    // It is important that this occurs in the correct triple-order!
    // In this unit test we order this manually, but eventually, this orderning is automatically by the patch tree.
    PatchPositions pos_1 = PatchPositions(); // This is an addition, so no patch positions
    PatchPositions pos_2 = Patch::positions(e_2.get_triple(), sp_, s_o, s__, _po, _p_, __o, ___);
    PatchPositions pos_3 = PatchPositions(); // This is an addition, so no patch positions
    PatchPositions pos_0 = Patch::positions(e_0.get_triple(), sp_, s_o, s__, _po, _p_, __o, ___);

    // All additions will have position -1, because these are not taken into account when determining positions!
    ASSERT_EQ(-1, pos_1.sp_) << "Found position is wrong";
    ASSERT_EQ(-1, pos_1.s__) << "Found position is wrong";
    ASSERT_EQ(-1, pos_1.s_o) << "Found position is wrong";
    ASSERT_EQ(-1, pos_1._po) << "Found position is wrong";
    ASSERT_EQ(-1, pos_1._p_) << "Found position is wrong";
    ASSERT_EQ(-1, pos_1.__o) << "Found position is wrong";
    ASSERT_EQ(-1, pos_1.___) << "Found position is wrong";

    ASSERT_EQ(-1, pos_3.sp_) << "Found position is wrong";
    ASSERT_EQ(-1, pos_3.s__) << "Found position is wrong";
    ASSERT_EQ(-1, pos_3.s_o) << "Found position is wrong";
    ASSERT_EQ(-1, pos_3._po) << "Found position is wrong";
    ASSERT_EQ(-1, pos_3._p_) << "Found position is wrong";
    ASSERT_EQ(-1, pos_3.__o) << "Found position is wrong";
    ASSERT_EQ(-1, pos_3.___) << "Found position is wrong";

    ASSERT_EQ(0, pos_0.sp_) << "Found position is wrong";
    ASSERT_EQ(0, pos_0.s__) << "Found position is wrong";
    ASSERT_EQ(0, pos_0.s_o) << "Found position is wrong";
    ASSERT_EQ(0, pos_0._po) << "Found position is wrong";
    ASSERT_EQ(0, pos_0._p_) << "Found position is wrong";
    ASSERT_EQ(1, pos_0.__o) << "Found position is wrong";
    ASSERT_EQ(1, pos_0.___) << "Found position is wrong";

    ASSERT_EQ(0, pos_2.sp_) << "Found position is wrong";
    ASSERT_EQ(0, pos_2.s__) << "Found position is wrong";
    ASSERT_EQ(0, pos_2.s_o) << "Found position is wrong";
    ASSERT_EQ(0, pos_2._po) << "Found position is wrong";
    ASSERT_EQ(0, pos_2._p_) << "Found position is wrong";
    ASSERT_EQ(0, pos_2.__o) << "Found position is wrong";
    ASSERT_EQ(0, pos_2.___) << "Found position is wrong";

    sp_.close();
    s_o.close();
    s__.close();
    _po.close();
    _p_.close();
    __o.close();

    std::remove(".additions.sp_.tmp");
    std::remove(".additions.s_o.tmp");
    std::remove(".additions.s__.tmp");
    std::remove(".additions._po.tmp");
    std::remove(".additions._p_.tmp");
    std::remove(".additions.__o.tmp");
}

TEST_F(PatchElementsTest, PositionPattern) {
    patchElements.add(PatchElement(Triple("s", "z", "o", dict), false));
    patchElements.add(PatchElement(Triple("g", "p", "o", dict), true));
    patchElements.add(PatchElement(Triple("q", "p", "o", dict), false));
    patchElements.add(PatchElement(Triple("s", "a", "o", dict), true));
    // Expected order:
    // g p o +
    // q p o -
    // s a o +
    // s z o -

    ASSERT_EQ(0, patchElements.position_of_pattern(PatchElement(Triple("g", "p", "o", dict), true ), false, false, false, false)) << "Found position is wrong";
    ASSERT_EQ(0, patchElements.position_of_pattern(PatchElement(Triple("g", "p", "o", dict), true ), false, false, false, true)) << "Found position is wrong";
    ASSERT_EQ(0, patchElements.position_of_pattern(PatchElement(Triple("g", "p", "o", dict), true ), false, false, true, false)) << "Found position is wrong";
    ASSERT_EQ(0, patchElements.position_of_pattern(PatchElement(Triple("g", "p", "o", dict), true ), false, false, true, true)) << "Found position is wrong";
    ASSERT_EQ(0, patchElements.position_of_pattern(PatchElement(Triple("g", "p", "o", dict), true ), false, true, false, false)) << "Found position is wrong";
    ASSERT_EQ(0, patchElements.position_of_pattern(PatchElement(Triple("g", "p", "o", dict), true ), false, true, false, true)) << "Found position is wrong";
    ASSERT_EQ(0, patchElements.position_of_pattern(PatchElement(Triple("g", "p", "o", dict), true ), false, true, true, false)) << "Found position is wrong";
    ASSERT_EQ(0, patchElements.position_of_pattern(PatchElement(Triple("g", "p", "o", dict), true ), false, true, true, true)) << "Found position is wrong";
    ASSERT_EQ(0, patchElements.position_of_pattern(PatchElement(Triple("g", "p", "o", dict), true ), true, false, false, false)) << "Found position is wrong";
    ASSERT_EQ(0, patchElements.position_of_pattern(PatchElement(Triple("g", "p", "o", dict), true ), true, false, false, true)) << "Found position is wrong";
    ASSERT_EQ(0, patchElements.position_of_pattern(PatchElement(Triple("g", "p", "o", dict), true ), true, false, true, false)) << "Found position is wrong";
    ASSERT_EQ(0, patchElements.position_of_pattern(PatchElement(Triple("g", "p", "o", dict), true ), true, false, true, true)) << "Found position is wrong";
    ASSERT_EQ(0, patchElements.position_of_pattern(PatchElement(Triple("g", "p", "o", dict), true ), true, true, false, false)) << "Found position is wrong";
    ASSERT_EQ(0, patchElements.position_of_pattern(PatchElement(Triple("g", "p", "o", dict), true ), true, true, false, true)) << "Found position is wrong";
    ASSERT_EQ(0, patchElements.position_of_pattern(PatchElement(Triple("g", "p", "o", dict), true ), true, true, true, false)) << "Found position is wrong";
    ASSERT_EQ(0, patchElements.position_of_pattern(PatchElement(Triple("g", "p", "o", dict), true ), true, true, true, true)) << "Found position is wrong";

    ASSERT_EQ(2, patchElements.position_of_pattern(PatchElement(Triple("s", "a", "o", dict), true ), false, false, false, false)) << "Found position is wrong";
    ASSERT_EQ(1, patchElements.position_of_pattern(PatchElement(Triple("s", "a", "o", dict), true ), false, false, false, true)) << "Found position is wrong";
    ASSERT_EQ(2, patchElements.position_of_pattern(PatchElement(Triple("s", "a", "o", dict), true ), false, false, true, false)) << "Found position is wrong";
    ASSERT_EQ(1, patchElements.position_of_pattern(PatchElement(Triple("s", "a", "o", dict), true ), false, false, true, true)) << "Found position is wrong";
    ASSERT_EQ(0, patchElements.position_of_pattern(PatchElement(Triple("s", "a", "o", dict), true ), false, true, false, false)) << "Found position is wrong";
    ASSERT_EQ(0, patchElements.position_of_pattern(PatchElement(Triple("s", "a", "o", dict), true ), false, true, false, true)) << "Found position is wrong";
    ASSERT_EQ(0, patchElements.position_of_pattern(PatchElement(Triple("s", "a", "o", dict), true ), false, true, true, false)) << "Found position is wrong";
    ASSERT_EQ(0, patchElements.position_of_pattern(PatchElement(Triple("s", "a", "o", dict), true ), false, true, true, true)) << "Found position is wrong";
    ASSERT_EQ(0, patchElements.position_of_pattern(PatchElement(Triple("s", "a", "o", dict), true ), true, false, false, false)) << "Found position is wrong";
    ASSERT_EQ(0, patchElements.position_of_pattern(PatchElement(Triple("s", "a", "o", dict), true ), true, false, false, true)) << "Found position is wrong";
    ASSERT_EQ(0, patchElements.position_of_pattern(PatchElement(Triple("s", "a", "o", dict), true ), true, false, true, false)) << "Found position is wrong";
    ASSERT_EQ(0, patchElements.position_of_pattern(PatchElement(Triple("s", "a", "o", dict), true ), true, false, true, true)) << "Found position is wrong";
    ASSERT_EQ(0, patchElements.position_of_pattern(PatchElement(Triple("s", "a", "o", dict), true ), true, true, false, false)) << "Found position is wrong";
    ASSERT_EQ(0, patchElements.position_of_pattern(PatchElement(Triple("s", "a", "o", dict), true ), true, true, false, true)) << "Found position is wrong";
    ASSERT_EQ(0, patchElements.position_of_pattern(PatchElement(Triple("s", "a", "o", dict), true ), true, true, true, false)) << "Found position is wrong";
    ASSERT_EQ(0, patchElements.position_of_pattern(PatchElement(Triple("s", "a", "o", dict), true ), true, true, true, true)) << "Found position is wrong";

    ASSERT_EQ(3, patchElements.position_of_pattern(PatchElement(Triple("s", "z", "o", dict), false), false, false, false, false)) << "Found position is wrong";
    ASSERT_EQ(1, patchElements.position_of_pattern(PatchElement(Triple("s", "z", "o", dict), false), false, false, false, true)) << "Found position is wrong";
    ASSERT_EQ(3, patchElements.position_of_pattern(PatchElement(Triple("s", "z", "o", dict), false), false, false, true, false)) << "Found position is wrong";
    ASSERT_EQ(1, patchElements.position_of_pattern(PatchElement(Triple("s", "z", "o", dict), false), false, false, true, true)) << "Found position is wrong";
    ASSERT_EQ(0, patchElements.position_of_pattern(PatchElement(Triple("s", "z", "o", dict), false), false, true, false, false)) << "Found position is wrong";
    ASSERT_EQ(0, patchElements.position_of_pattern(PatchElement(Triple("s", "z", "o", dict), false), false, true, false, true)) << "Found position is wrong";
    ASSERT_EQ(0, patchElements.position_of_pattern(PatchElement(Triple("s", "z", "o", dict), false), false, true, true, false)) << "Found position is wrong";
    ASSERT_EQ(0, patchElements.position_of_pattern(PatchElement(Triple("s", "z", "o", dict), false), false, true, true, true)) << "Found position is wrong";
    ASSERT_EQ(1, patchElements.position_of_pattern(PatchElement(Triple("s", "z", "o", dict), false), true, false, false, false)) << "Found position is wrong";
    ASSERT_EQ(0, patchElements.position_of_pattern(PatchElement(Triple("s", "z", "o", dict), false), true, false, false, true)) << "Found position is wrong";
    ASSERT_EQ(1, patchElements.position_of_pattern(PatchElement(Triple("s", "z", "o", dict), false), true, false, true, false)) << "Found position is wrong";
    ASSERT_EQ(0, patchElements.position_of_pattern(PatchElement(Triple("s", "z", "o", dict), false), true, false, true, true)) << "Found position is wrong";
    ASSERT_EQ(0, patchElements.position_of_pattern(PatchElement(Triple("s", "z", "o", dict), false), true, true, false, false)) << "Found position is wrong";
    ASSERT_EQ(0, patchElements.position_of_pattern(PatchElement(Triple("s", "z", "o", dict), false), true, true, false, true)) << "Found position is wrong";
    ASSERT_EQ(0, patchElements.position_of_pattern(PatchElement(Triple("s", "z", "o", dict), false), true, true, true, false)) << "Found position is wrong";
    ASSERT_EQ(0, patchElements.position_of_pattern(PatchElement(Triple("s", "z", "o", dict), false), true, true, true, true)) << "Found position is wrong";

    ASSERT_EQ(0,  patchElements.position_of_pattern(PatchElement(Triple("g", "p", "o", dict), false), false, false, false, false)) << "Found position is wrong";
    ASSERT_EQ(-1, patchElements.position_of_pattern(PatchElement(Triple("g", "p", "o", dict), false), false, false, false, true)) << "Found position is wrong";
    ASSERT_EQ(0,  patchElements.position_of_pattern(PatchElement(Triple("g", "p", "o", dict), false), false, false, true, false)) << "Found position is wrong";
    ASSERT_EQ(-1, patchElements.position_of_pattern(PatchElement(Triple("g", "p", "o", dict), false), false, false, true, true)) << "Found position is wrong";
    ASSERT_EQ(0,  patchElements.position_of_pattern(PatchElement(Triple("g", "p", "o", dict), false), false, true, false, false)) << "Found position is wrong";
    ASSERT_EQ(-1, patchElements.position_of_pattern(PatchElement(Triple("g", "p", "o", dict), false), false, true, false, true)) << "Found position is wrong";
    ASSERT_EQ(0,  patchElements.position_of_pattern(PatchElement(Triple("g", "p", "o", dict), false), false, true, true, false)) << "Found position is wrong";
    ASSERT_EQ(-1, patchElements.position_of_pattern(PatchElement(Triple("g", "p", "o", dict), false), false, true, true, true)) << "Found position is wrong";
    ASSERT_EQ(0,  patchElements.position_of_pattern(PatchElement(Triple("g", "p", "o", dict), false), true, false, false, false)) << "Found position is wrong";
    ASSERT_EQ(-1, patchElements.position_of_pattern(PatchElement(Triple("g", "p", "o", dict), false), true, false, false, true)) << "Found position is wrong";
    ASSERT_EQ(0,  patchElements.position_of_pattern(PatchElement(Triple("g", "p", "o", dict), false), true, false, true, false)) << "Found position is wrong";
    ASSERT_EQ(-1, patchElements.position_of_pattern(PatchElement(Triple("g", "p", "o", dict), false), true, false, true, true)) << "Found position is wrong";
    ASSERT_EQ(0,  patchElements.position_of_pattern(PatchElement(Triple("g", "p", "o", dict), false), true, true, false, false)) << "Found position is wrong";
    ASSERT_EQ(-1, patchElements.position_of_pattern(PatchElement(Triple("g", "p", "o", dict), false), true, true, false, true)) << "Found position is wrong";
    ASSERT_EQ(0,  patchElements.position_of_pattern(PatchElement(Triple("g", "p", "o", dict), false), true, true, true, false)) << "Found position is wrong";
    ASSERT_EQ(-1, patchElements.position_of_pattern(PatchElement(Triple("g", "p", "o", dict), false), true, true, true, true)) << "Found position is wrong";

    ASSERT_EQ(0,  patchElements.position_of_pattern(PatchElement(Triple("a", "a", "a", dict), false), false, false, false, false)) << "Found position is wrong";
    ASSERT_EQ(-1, patchElements.position_of_pattern(PatchElement(Triple("a", "a", "a", dict), false), false, false, false, true)) << "Found position is wrong";
    ASSERT_EQ(-1, patchElements.position_of_pattern(PatchElement(Triple("a", "a", "a", dict), false), false, false, true, false)) << "Found position is wrong";
    ASSERT_EQ(-1, patchElements.position_of_pattern(PatchElement(Triple("a", "a", "a", dict), false), false, false, true, true)) << "Found position is wrong";
    ASSERT_EQ(-1, patchElements.position_of_pattern(PatchElement(Triple("a", "a", "a", dict), false), false, true, false, false)) << "Found position is wrong";
    ASSERT_EQ(-1, patchElements.position_of_pattern(PatchElement(Triple("a", "a", "a", dict), false), false, true, false, true)) << "Found position is wrong";
    ASSERT_EQ(-1, patchElements.position_of_pattern(PatchElement(Triple("a", "a", "a", dict), false), false, true, true, false)) << "Found position is wrong";
    ASSERT_EQ(-1, patchElements.position_of_pattern(PatchElement(Triple("a", "a", "a", dict), false), false, true, true, true)) << "Found position is wrong";
    ASSERT_EQ(-1, patchElements.position_of_pattern(PatchElement(Triple("a", "a", "a", dict), false), true, false, false, false)) << "Found position is wrong";
    ASSERT_EQ(-1, patchElements.position_of_pattern(PatchElement(Triple("a", "a", "a", dict), false), true, false, false, true)) << "Found position is wrong";
    ASSERT_EQ(-1, patchElements.position_of_pattern(PatchElement(Triple("a", "a", "a", dict), false), true, false, true, false)) << "Found position is wrong";
    ASSERT_EQ(-1, patchElements.position_of_pattern(PatchElement(Triple("a", "a", "a", dict), false), true, false, true, true)) << "Found position is wrong";
    ASSERT_EQ(-1, patchElements.position_of_pattern(PatchElement(Triple("a", "a", "a", dict), false), true, true, false, false)) << "Found position is wrong";
    ASSERT_EQ(-1, patchElements.position_of_pattern(PatchElement(Triple("a", "a", "a", dict), false), true, true, false, true)) << "Found position is wrong";
    ASSERT_EQ(-1, patchElements.position_of_pattern(PatchElement(Triple("a", "a", "a", dict), false), true, true, true, false)) << "Found position is wrong";
    ASSERT_EQ(-1, patchElements.position_of_pattern(PatchElement(Triple("a", "a", "a", dict), false), true, true, true, true)) << "Found position is wrong";
}

TEST_F(PatchElementsTest, PositionNotPresent) {
    patchElements.add(PatchElement(Triple("s", "z", "o", dict), false));
    patchElements.add(PatchElement(Triple("g", "p", "o", dict), true));
    patchElements.add(PatchElement(Triple("q", "p", "o", dict), false));
    patchElements.add(PatchElement(Triple("s", "a", "o", dict), true));

    ASSERT_EQ(0, patchElements.position_of(PatchElement(Triple("g", "p", "o", dict), false))) << "Found position is wrong";
    ASSERT_EQ(1, patchElements.position_of(PatchElement(Triple("q", "p", "o", dict), true ))) << "Found position is wrong";
    ASSERT_EQ(0, patchElements.position_of(PatchElement(Triple("a", "a", "a", dict), true ))) << "Found position is wrong";
    ASSERT_EQ(4, patchElements.position_of(PatchElement(Triple("s", "z", "z", dict), false))) << "Found position is wrong";
}

TEST_F(PatchElementsTest, PositionStrict) {
    patchElements.add(PatchElement(Triple("s", "z", "o", dict), false));
    patchElements.add(PatchElement(Triple("g", "p", "o", dict), true));
    patchElements.add(PatchElement(Triple("q", "p", "o", dict), false));
    patchElements.add(PatchElement(Triple("s", "a", "o", dict), true));

    ASSERT_EQ(0, patchElements.position_of_strict(PatchElement(Triple("g", "p", "o", dict), true ))) << "Found position is wrong";
    ASSERT_EQ(1, patchElements.position_of_strict(PatchElement(Triple("q", "p", "o", dict), false))) << "Found position is wrong";
    ASSERT_EQ(2, patchElements.position_of_strict(PatchElement(Triple("s", "a", "o", dict), true ))) << "Found position is wrong";
    ASSERT_EQ(3, patchElements.position_of_strict(PatchElement(Triple("s", "z", "o", dict), false))) << "Found position is wrong";

    ASSERT_EQ(-1, patchElements.position_of_strict(PatchElement(Triple("g", "p", "o", dict), false))) << "Found position is wrong";
    ASSERT_EQ(-1, patchElements.position_of_strict(PatchElement(Triple("a", "a", "a", dict), false))) << "Found position is wrong";
}

TEST_F(PatchElementsTest, ApplyLocalChanges) {
    PatchSorted p1(dict);
    p1.add(PatchElement(Triple("a", "a", "a", dict), false));
    ASSERT_EQ("a a a. (-)\n", p1.to_string(*dict));

    PatchSorted p2(dict);
    p2.add(PatchElement(Triple("a", "a", "a", dict), false));
    p2.add(PatchElement(Triple("a", "a", "a", dict), true));
    ASSERT_EQ("a a a. (+) L\n", p2.to_string(*dict));

    PatchSorted p3(dict);
    PatchElement p3e = PatchElement(Triple("a", "a", "a", dict), false);
    p3e.set_local_change(true);
    p3.add(p3e);
    p3.add(PatchElement(Triple("a", "a", "a", dict), true));
    ASSERT_EQ("a a a. (+)\n", p3.to_string(*dict));

    PatchSorted p4(dict);
    PatchElement p4e = PatchElement(Triple("a", "a", "a", dict), true);
    p4e.set_local_change(true);
    p4.add(p4e);
    p4.add(PatchElement(Triple("a", "a", "a", dict), false));
    ASSERT_EQ("a a a. (-)\n", p4.to_string(*dict));

    PatchSorted p5(dict);
    p5.add(PatchElement(Triple("a", "a", "a", dict), true));
    p5.add(PatchElement(Triple("a", "a", "a", dict), false));
    ASSERT_EQ("a a a. (-) L\n", p5.to_string(*dict));

    PatchSorted p6(dict);
    p6.add(PatchElement(Triple("a", "a", "a", dict), true));
    p6.add(PatchElement(Triple("a", "a", "a", dict), false));
    p6.add(PatchElement(Triple("a", "a", "a", dict), true));
    ASSERT_EQ("a a a. (+)\n", p6.to_string(*dict));

    PatchSorted p7(dict);
    p7.add(PatchElement(Triple("a", "a", "a", dict), true));
    p7.add(PatchElement(Triple("a", "a", "a", dict), false));
    p7.add(PatchElement(Triple("a", "a", "a", dict), true));
    p7.add(PatchElement(Triple("a", "a", "a", dict), false));
    ASSERT_EQ("a a a. (-) L\n", p7.to_string(*dict));
}

TEST_F(PatchElementsTest, JoinSorted) {
    PatchTreeKeyComparator comp_key(comp_s, comp_p, comp_o, dict);
    PatchElementComparator comp(&comp_key);
    PatchHashed patch_existing;
    patch_existing.add(PatchElement(Triple("a", "a", "a", dict), true));
    patch_existing.add(PatchElement(Triple("a", "a", "b", dict), false));

    PatchSorted patch_new(dict);
    patch_new.add(PatchElement(Triple("a", "a", "a", dict), false));
    patch_new.add(PatchElement(Triple("a", "a", "c", dict), false));

    PatchSorted* patch_combined = patch_existing.join_sorted(patch_new, &comp);
    ASSERT_EQ("a a a. (-) L\n"
              "a a b. (-)\n"
              "a a c. (-)\n", patch_combined->to_string(*dict));
    delete patch_combined;
}
