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

TEST_F(PatchElementsTest, GetSize) {
    ASSERT_EQ(0, patchElements.getSize()) << "Size of empty patch must be 0";

    patchElements.add(PatchElement(Triple("s1", "p1", "o1"), true));
    ASSERT_EQ(1, patchElements.getSize()) << "Size of patch with one element must be 1";

    patchElements.add(PatchElement(Triple("s2", "p2", "o2"), false));
    patchElements.add(PatchElement(Triple("s3", "p3", "o3"), false));
    patchElements.add(PatchElement(Triple("s4", "p4", "o4"), true));
    ASSERT_EQ(4, patchElements.getSize()) << "Size of patch with four elements must be 4";
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