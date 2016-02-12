#include <gtest/gtest.h>

#include "../../../main/cpp/patch/patch_tree.h"

// The fixture for testing class Patch.
class PatchElementsTest : public ::testing::Test {
protected:
    Patch patchElements;

    PatchElementsTest() : patchElements() {}

    virtual void SetUp() {
        patchElements = Patch();
    }
};

TEST_F(PatchElementsTest, AddSingle) {
    patchElements.add(PatchElement(Triple("s1", "p1", "o1"), true));
}

TEST_F(PatchElementsTest, AddMultiple) {
    patchElements.add(PatchElement(Triple("s1", "p1", "o1"), true));
    patchElements.add(PatchElement(Triple("s2", "p2", "o2"), false));
    patchElements.add(PatchElement(Triple("s3", "p3", "o3"), false));
    patchElements.add(PatchElement(Triple("s4", "p4", "o4"), true));
}

TEST_F(PatchElementsTest, AddAll) {
    Patch patch2;

    patchElements.add(PatchElement(Triple("s1", "p1", "o1"), true));
    patchElements.add(PatchElement(Triple("s3", "p3", "o3"), false));

    patch2.add(PatchElement(Triple("s2", "p2", "o2"), false));
    patch2.add(PatchElement(Triple("s4", "p4", "o4"), true));
    patchElements.addAll(patch2);

    ASSERT_EQ("s1 p1 o1. (+)", patchElements.get(0).to_string()) << "First element is incorrect";
    ASSERT_EQ("s2 p2 o2. (-)", patchElements.get(1).to_string()) << "Second element is incorrect";
    ASSERT_EQ("s3 p3 o3. (-)", patchElements.get(2).to_string()) << "Third element is incorrect";
    ASSERT_EQ("s4 p4 o4. (+)", patchElements.get(3).to_string()) << "Fourth element is incorrect";
}

TEST_F(PatchElementsTest, ToString) {
    patchElements.add(PatchElement(Triple("s1", "p1", "o1"), true));
    patchElements.add(PatchElement(Triple("s2", "p2", "o2"), false));
    patchElements.add(PatchElement(Triple("s3", "p3", "o3"), false));
    patchElements.add(PatchElement(Triple("s4", "p4", "o4"), true));

    ASSERT_EQ("s1 p1 o1. (+)\n"
              "s2 p2 o2. (-)\n"
              "s3 p3 o3. (-)\n"
              "s4 p4 o4. (+)\n", patchElements.to_string()) << "to_string() is invalid";
}

TEST_F(PatchElementsTest, GetSize) {
    ASSERT_EQ(0, patchElements.get_size()) << "Size of empty patch must be 0";

    patchElements.add(PatchElement(Triple("s1", "p1", "o1"), true));
    ASSERT_EQ(1, patchElements.get_size()) << "Size of patch with one element must be 1";

    patchElements.add(PatchElement(Triple("s2", "p2", "o2"), false));
    patchElements.add(PatchElement(Triple("s3", "p3", "o3"), false));
    patchElements.add(PatchElement(Triple("s4", "p4", "o4"), true));
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
    patchElements.add(PatchElement(Triple("s1", "p1", "o1"), true));
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
    PatchElement patchElement1(Triple("s1", "p1", "o1"), true);
    patchElements.add(patchElement1);
    PatchElement patchElement1Got = patchElements.get(0);

    ASSERT_EQ(patchElement1.is_addition(), patchElement1Got.is_addition()) << "Retrieved patch type does not equal the inserted patch type";
    ASSERT_EQ(patchElement1.get_triple().get_subject(), patchElement1Got.get_triple().get_subject()) << "Retrieved patch subject does not equal the inserted patch subject";
    ASSERT_EQ(patchElement1.get_triple().get_predicate(), patchElement1Got.get_triple().get_predicate()) << "Retrieved patch predicate does not equal the inserted patch predicate";
    ASSERT_EQ(patchElement1.get_triple().get_object(), patchElement1Got.get_triple().get_object()) << "Retrieved patch object does not equal the inserted patch object";
}

TEST_F(PatchElementsTest, GetMultiple) {
    PatchElement patchElement1(Triple("s1", "p1", "o1"), true);
    PatchElement patchElement2(Triple("s2", "p2", "o2"), false);
    PatchElement patchElement3(Triple("s3", "p3", "o3"), false);
    PatchElement patchElement4(Triple("s4", "p4", "o4"), true);
    patchElements.add(patchElement1);
    patchElements.add(patchElement2);
    patchElements.add(patchElement3);
    patchElements.add(patchElement4);
    PatchElement patchElement1Got = patchElements.get(0);
    PatchElement patchElement2Got = patchElements.get(1);
    PatchElement patchElement3Got = patchElements.get(2);
    PatchElement patchElement4Got = patchElements.get(3);

    ASSERT_EQ(patchElement1.is_addition(), patchElement1Got.is_addition()) << "Retrieved patch 1 type does not equal the inserted patch type";
    ASSERT_EQ(patchElement1.get_triple().get_subject(), patchElement1Got.get_triple().get_subject()) << "Retrieved patch 1 subject does not equal the inserted patch subject";
    ASSERT_EQ(patchElement1.get_triple().get_predicate(), patchElement1Got.get_triple().get_predicate()) << "Retrieved patch 1 predicate does not equal the inserted patch predicate";
    ASSERT_EQ(patchElement1.get_triple().get_object(), patchElement1Got.get_triple().get_object()) << "Retrieved patch 1 object does not equal the inserted patch object";

    ASSERT_EQ(patchElement2.is_addition(), patchElement2Got.is_addition()) << "Retrieved patch 2 type does not equal the inserted patch type";
    ASSERT_EQ(patchElement2.get_triple().get_subject(), patchElement2Got.get_triple().get_subject()) << "Retrieved patch 2 subject does not equal the inserted patch subject";
    ASSERT_EQ(patchElement2.get_triple().get_predicate(), patchElement2Got.get_triple().get_predicate()) << "Retrieved patch 2 predicate does not equal the inserted patch predicate";
    ASSERT_EQ(patchElement2.get_triple().get_object(), patchElement2Got.get_triple().get_object()) << "Retrieved patch 2 object does not equal the inserted patch object";

    ASSERT_EQ(patchElement3.is_addition(), patchElement3Got.is_addition()) << "Retrieved patch 3 type does not equal the inserted patch type";
    ASSERT_EQ(patchElement3.get_triple().get_subject(), patchElement3Got.get_triple().get_subject()) << "Retrieved patch 3 subject does not equal the inserted patch subject";
    ASSERT_EQ(patchElement3.get_triple().get_predicate(), patchElement3Got.get_triple().get_predicate()) << "Retrieved patch 3 predicate does not equal the inserted patch predicate";
    ASSERT_EQ(patchElement3.get_triple().get_object(), patchElement3Got.get_triple().get_object()) << "Retrieved patch 3 object does not equal the inserted patch object";

    ASSERT_EQ(patchElement4.is_addition(), patchElement4Got.is_addition()) << "Retrieved patch 4 type does not equal the inserted patch type";
    ASSERT_EQ(patchElement4.get_triple().get_subject(), patchElement4Got.get_triple().get_subject()) << "Retrieved patch 4 subject does not equal the inserted patch subject";
    ASSERT_EQ(patchElement4.get_triple().get_predicate(), patchElement4Got.get_triple().get_predicate()) << "Retrieved patch 4 predicate does not equal the inserted patch predicate";
    ASSERT_EQ(patchElement4.get_triple().get_object(), patchElement4Got.get_triple().get_object()) << "Retrieved patch 4 object does not equal the inserted patch object";
}

TEST_F(PatchElementsTest, Order1) {
    patchElements.add(PatchElement(Triple("s3", "p3", "o3"), false));
    patchElements.add(PatchElement(Triple("s1", "p1", "o1"), false));
    patchElements.add(PatchElement(Triple("s4", "p4", "o4"), true));
    patchElements.add(PatchElement(Triple("s2", "p2", "o2"), true));

    ASSERT_EQ("s1 p1 o1. (-)", patchElements.get(0).to_string()) << "First element is incorrect";
    ASSERT_EQ("s2 p2 o2. (+)", patchElements.get(1).to_string()) << "Second element is incorrect";
    ASSERT_EQ("s3 p3 o3. (-)", patchElements.get(2).to_string()) << "Third element is incorrect";
    ASSERT_EQ("s4 p4 o4. (+)", patchElements.get(3).to_string()) << "Fourth element is incorrect";
}

TEST_F(PatchElementsTest, Order2) {
    patchElements.add(PatchElement(Triple("s4", "p4", "o4"), true));
    patchElements.add(PatchElement(Triple("s3", "p3", "o3"), false));
    patchElements.add(PatchElement(Triple("s2", "p2", "o2"), true));
    patchElements.add(PatchElement(Triple("s1", "p1", "o1"), false));

    ASSERT_EQ("s1 p1 o1. (-)", patchElements.get(0).to_string()) << "First element is incorrect";
    ASSERT_EQ("s2 p2 o2. (+)", patchElements.get(1).to_string()) << "Second element is incorrect";
    ASSERT_EQ("s3 p3 o3. (-)", patchElements.get(2).to_string()) << "Third element is incorrect";
    ASSERT_EQ("s4 p4 o4. (+)", patchElements.get(3).to_string()) << "Fourth element is incorrect";
}

TEST_F(PatchElementsTest, Order3) {
    patchElements.add(PatchElement(Triple("q", "p", "o"), false));
    patchElements.add(PatchElement(Triple("g", "p", "o"), true));
    patchElements.add(PatchElement(Triple("s", "z", "o"), false));
    patchElements.add(PatchElement(Triple("s", "a", "o"), true));

    ASSERT_EQ("g p o. (+)", patchElements.get(0).to_string()) << "First element is incorrect";
    ASSERT_EQ("q p o. (-)", patchElements.get(1).to_string()) << "Second element is incorrect";
    ASSERT_EQ("s a o. (+)", patchElements.get(2).to_string()) << "Third element is incorrect";
    ASSERT_EQ("s z o. (-)", patchElements.get(3).to_string()) << "Fourth element is incorrect";
}

TEST_F(PatchElementsTest, Order4) {
    patchElements.add(PatchElement(Triple("s", "z", "o"), false));
    patchElements.add(PatchElement(Triple("g", "p", "o"), true));
    patchElements.add(PatchElement(Triple("q", "p", "o"), false));
    patchElements.add(PatchElement(Triple("s", "a", "o"), true));

    ASSERT_EQ("g p o. (+)", patchElements.get(0).to_string()) << "First element is incorrect";
    ASSERT_EQ("q p o. (-)", patchElements.get(1).to_string()) << "Second element is incorrect";
    ASSERT_EQ("s a o. (+)", patchElements.get(2).to_string()) << "Third element is incorrect";
    ASSERT_EQ("s z o. (-)", patchElements.get(3).to_string()) << "Fourth element is incorrect";
}

TEST_F(PatchElementsTest, PositionPresent) {
    patchElements.add(PatchElement(Triple("s", "z", "o"), false));
    patchElements.add(PatchElement(Triple("g", "p", "o"), true));
    patchElements.add(PatchElement(Triple("q", "p", "o"), false));
    patchElements.add(PatchElement(Triple("s", "a", "o"), true));

    ASSERT_EQ(0, patchElements.position_of(PatchElement(Triple("g", "p", "o"), true ))) << "Found position is wrong";
    ASSERT_EQ(1, patchElements.position_of(PatchElement(Triple("q", "p", "o"), false))) << "Found position is wrong";
    ASSERT_EQ(2, patchElements.position_of(PatchElement(Triple("s", "a", "o"), true ))) << "Found position is wrong";
    ASSERT_EQ(3, patchElements.position_of(PatchElement(Triple("s", "z", "o"), false))) << "Found position is wrong";
}

TEST_F(PatchElementsTest, PositionNotPresent) {
    patchElements.add(PatchElement(Triple("s", "z", "o"), false));
    patchElements.add(PatchElement(Triple("g", "p", "o"), true));
    patchElements.add(PatchElement(Triple("q", "p", "o"), false));
    patchElements.add(PatchElement(Triple("s", "a", "o"), true));

    ASSERT_EQ(0, patchElements.position_of(PatchElement(Triple("g", "p", "o"), false))) << "Found position is wrong";
    ASSERT_EQ(2, patchElements.position_of(PatchElement(Triple("q", "p", "o"), true ))) << "Found position is wrong";
    ASSERT_EQ(0, patchElements.position_of(PatchElement(Triple("a", "a", "a"), true ))) << "Found position is wrong";
    ASSERT_EQ(4, patchElements.position_of(PatchElement(Triple("s", "z", "z"), false))) << "Found position is wrong";
}

TEST_F(PatchElementsTest, PositionStrict) {
    patchElements.add(PatchElement(Triple("s", "z", "o"), false));
    patchElements.add(PatchElement(Triple("g", "p", "o"), true));
    patchElements.add(PatchElement(Triple("q", "p", "o"), false));
    patchElements.add(PatchElement(Triple("s", "a", "o"), true));

    ASSERT_EQ(0, patchElements.position_of_strict(PatchElement(Triple("g", "p", "o"), true ))) << "Found position is wrong";
    ASSERT_EQ(1, patchElements.position_of_strict(PatchElement(Triple("q", "p", "o"), false))) << "Found position is wrong";
    ASSERT_EQ(2, patchElements.position_of_strict(PatchElement(Triple("s", "a", "o"), true ))) << "Found position is wrong";
    ASSERT_EQ(3, patchElements.position_of_strict(PatchElement(Triple("s", "z", "o"), false))) << "Found position is wrong";

    ASSERT_EQ(-1, patchElements.position_of_strict(PatchElement(Triple("g", "p", "o"), false))) << "Found position is wrong";
    ASSERT_EQ(-1, patchElements.position_of_strict(PatchElement(Triple("a", "a", "a"), false))) << "Found position is wrong";
}
