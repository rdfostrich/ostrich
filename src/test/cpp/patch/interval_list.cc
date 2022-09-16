#include <gtest/gtest.h>

#include "../../../main/cpp/patch/interval_list.h"
#include "../../../main/cpp/patch/patch_tree_deletion_value.h"


TEST(IntervalListTest, SimpleAddition) {
    IntervalList<int> list(2);
    list.addition(1);
    ASSERT_TRUE(list.is_in(1)) << "1 should be in the list";
    ASSERT_FALSE(list.is_in(0)) << "0 should not be in the list";
}

TEST(IntervalListTest, SimpleAdditionDeletion) {
    IntervalList<int> list(3);
    list.addition(1);
    list.deletion(2);
    ASSERT_TRUE(list.is_in(1)) << "1 should be in the list";
    ASSERT_FALSE(list.is_in(0)) << "0 should not be in the list";
    ASSERT_FALSE(list.is_in(2)) << "2 should not be in the list";
    ASSERT_FALSE(list.is_in(3)) << "3 should not be in the list";
}

TEST(IntervalListTest, SimpleReinsertion) {
    IntervalList<int> list(3);
    list.addition(1);
    list.deletion(2);
    list.addition(2);
    ASSERT_TRUE(list.is_in(1)) << "1 should be in the list";
    ASSERT_FALSE(list.is_in(0)) << "0 should not be in the list";
    ASSERT_TRUE(list.is_in(2)) << "2 should be in the list";
}

TEST(IntervalListTest, MultipleIntervals1) {
    IntervalList<int> list(5);
    list.addition(1);
    list.deletion(2);
    list.addition(3);
    ASSERT_TRUE(list.is_in(1)) << "1 should be in the list";
    ASSERT_FALSE(list.is_in(0)) << "0 should not be in the list";
    ASSERT_FALSE(list.is_in(2)) << "2 should not be in the list";
    ASSERT_TRUE(list.is_in(3)) << "3 should be in the list";
    ASSERT_TRUE(list.is_in(4)) << "4 should be in the list";
}

TEST(IntervalListTest, MultipleIntervals2) {
    IntervalList<int> list(5);
    list.addition(1);
    list.deletion(2);
    list.addition(3);
    list.deletion(4);
    ASSERT_TRUE(list.is_in(1)) << "1 should be in the list";
    ASSERT_FALSE(list.is_in(0)) << "0 should not be in the list";
    ASSERT_FALSE(list.is_in(2)) << "2 should not be in the list";
    ASSERT_TRUE(list.is_in(3)) << "3 should be in the list";
    ASSERT_FALSE(list.is_in(4)) << "4 should not be in the list";
}

TEST(IntervalListTest, RevertDeletion1) {
    IntervalList<int> list(5);
    list.addition(1);
    list.deletion(2);
    list.addition(3);
    list.deletion(4);
    list.addition(2);
    ASSERT_TRUE(list.is_in(1)) << "1 should be in the list";
    ASSERT_FALSE(list.is_in(0)) << "0 should not be in the list";
    ASSERT_TRUE(list.is_in(2)) << "2 should be in the list";
    ASSERT_TRUE(list.is_in(3)) << "3 should be in the list";
    ASSERT_FALSE(list.is_in(4)) << "4 should not be in the list";
}

TEST(IntervalListTest, InnerAddition1) {
    IntervalList<int> list(6);
    list.addition(1);
    list.deletion(2);
    list.addition(4);
    list.deletion(5);
    list.addition(3);
    ASSERT_TRUE(list.is_in(1)) << "1 should be in the list";
    ASSERT_FALSE(list.is_in(0)) << "0 should not be in the list";
    ASSERT_FALSE(list.is_in(2)) << "2 should not be in the list";
    ASSERT_TRUE(list.is_in(3)) << "3 should be in the list";
    ASSERT_TRUE(list.is_in(4)) << "4 should be in the list";
    ASSERT_FALSE(list.is_in(5)) << "5 should not be in the list";
}

TEST(IntervalListTest, Serialization1) {
    IntervalList<int> list(2);
    list.addition(1);
    std::pair<const char*, size_t> s = list.serialize();
    list.deserialize(s.first, s.second);
    ASSERT_TRUE(list.is_in(1)) << "1 should be in the list";
    ASSERT_FALSE(list.is_in(0)) << "0 should not be in the list";
    delete[] s.first;
}

TEST(IntervalListTest, Serialization2) {
    IntervalList<int> list(7);
    list.addition(1);
    list.deletion(2);
    list.addition(4);
    list.deletion(6);
    std::pair<const char*, size_t> s = list.serialize();
    IntervalList<int> list2(6);
    list2.deserialize(s.first, s.second);
    ASSERT_TRUE(list2.is_in(1)) << "1 should be in the list";
    ASSERT_FALSE(list2.is_in(0)) << "0 should not be in the list";
    ASSERT_FALSE(list2.is_in(3)) << "3 should not be in the list";
    ASSERT_TRUE(list2.is_in(4)) << "4 should be in the list";
    ASSERT_TRUE(list2.is_in(5)) << "5 should be in the list";
    ASSERT_FALSE(list2.is_in(6)) << "6 should not be in the list";
    delete[] s.first;
}

TEST(IntervalListTest, DoubleAddition1) {
    IntervalList<int> list(10);
    list.addition(1);
    list.deletion(2);
    list.addition(4);
    list.addition(4);
    ASSERT_TRUE(list.is_in(1)) << "1 should be in the list";
    ASSERT_FALSE(list.is_in(0)) << "0 should not be in the list";
    ASSERT_FALSE(list.is_in(2)) << "2 should not be in the list";
    ASSERT_TRUE(list.is_in(4)) << "4 should be in the list";
    ASSERT_TRUE(list.is_in(5)) << "5 should be in the list";
    ASSERT_TRUE(list.is_in(6)) << "6 should be in the list";
}

TEST(IntervalListTest, DoubleAddition2) {
    IntervalList<int> list(10);
    list.addition(1);
    list.deletion(2);
    list.addition(4);
    list.deletion(4);
    list.addition(4);
    ASSERT_TRUE(list.is_in(1)) << "1 should be in the list";
    ASSERT_FALSE(list.is_in(0)) << "0 should not be in the list";
    ASSERT_FALSE(list.is_in(2)) << "2 should not be in the list";
    ASSERT_TRUE(list.is_in(4)) << "4 should be in the list";
    ASSERT_TRUE(list.is_in(5)) << "5 should be in the list";
    ASSERT_TRUE(list.is_in(6)) << "6 should be in the list";
}

TEST(IntervalListTest, IntervalDeletion) {
    IntervalList<int> list(5);
    list.addition(1);
    list.deletion(2);
    list.addition(3);
    list.deletion(4);
    list.deletion(3);
    ASSERT_TRUE(list.is_in(1)) << "1 should be in the list";
    ASSERT_FALSE(list.is_in(0)) << "0 should not be in the list";
    ASSERT_FALSE(list.is_in(2)) << "2 should not be in the list";
    ASSERT_FALSE(list.is_in(3)) << "3 should not be in the list";
    ASSERT_FALSE(list.is_in(4)) << "4 should not be in the list";
}

TEST(IntervalListTest, IntervalModification) {
    IntervalList<int> list(8);
    list.addition(1);
    list.deletion(2);
    list.addition(3);
    list.deletion(5);
    list.addition(6);
    list.deletion(7);
    list.deletion(4);
    ASSERT_TRUE(list.is_in(1)) << "1 should be in the list";
    ASSERT_FALSE(list.is_in(0)) << "0 should not be in the list";
    ASSERT_FALSE(list.is_in(2)) << "2 should not be in the list";
    ASSERT_TRUE(list.is_in(3)) << "3 should be in the list";
    ASSERT_FALSE(list.is_in(4)) << "4 should not be in the list";
    ASSERT_TRUE(list.is_in(6)) << "6 should be in the list";
    ASSERT_FALSE(list.is_in(7)) << "7 should not be in the list";
}

TEST(IntervalListTest, EmptyDeletion) {
    IntervalList<int> list(3);
    list.deletion(2);
    ASSERT_FALSE(list.is_in(0)) << "0 should not be in the list";
    ASSERT_FALSE(list.is_in(1)) << "1 should not be in the list";
    ASSERT_FALSE(list.is_in(2)) << "2 should not be in the list";
    ASSERT_FALSE(list.is_in(3)) << "3 should not be in the list";
}


// DeletionValue Interval List tests

TEST(DVIntervalListTest, SimpleAddition1) {
    DVIntervalList<PatchTreeDeletionValueElement, int, PatchPositions> dvl;
    PatchTreeDeletionValueElement pe(1);
    dvl.addition(pe);
    long index = dvl.get_index(1, 2);
    ASSERT_TRUE(index == 0) << "The element's index should be 0";
}

TEST(DVIntervalListTest, SimpleAddition2) {
    DVIntervalList<PatchTreeDeletionValueElement, int, PatchPositions> dvl;
    PatchTreeDeletionValueElement pe(1);
    dvl.addition(pe);
    long index = dvl.get_index(1, 2);
    PatchTreeDeletionValueElement pe2 = dvl.get_element_at(index, 2);
    ASSERT_TRUE(pe == pe2) << "The elements should be equal";
}

TEST(DVIntervalListTest, Addition1) {
    DVIntervalList<PatchTreeDeletionValueElement, int, PatchPositions> dvl;
    PatchTreeDeletionValueElement pe(1);
    dvl.addition(pe);
    dvl.deletion(5);
    PatchTreeDeletionValueElement pe3 = dvl.get_element_at(2, 6);
    ASSERT_TRUE(pe.get_patch_positions() == pe3.get_patch_positions()) << "The elements should be equal";
}

TEST(DVIntervalListTest, Addition2) {
    DVIntervalList<PatchTreeDeletionValueElement, int, PatchPositions> dvl;
    PatchTreeDeletionValueElement pe(1);
    dvl.addition(pe);
    PatchTreeDeletionValueElement pe2(4, PatchPositions(0,2,3,4,5,6, 7));
    dvl.addition(pe2);
    PatchTreeDeletionValueElement pe3 = dvl.get_element_at(2, 6);
    PatchTreeDeletionValueElement pe4 = dvl.get_element_at(4, 6);
    ASSERT_TRUE(pe.get_patch_positions() == pe3.get_patch_positions()) << "The elements should be equal";
    ASSERT_FALSE(pe.get_patch_positions() == pe4.get_patch_positions()) << "The elements should not be equal";
}

TEST(DVIntervalListTest, Addition3) {
    DVIntervalList<PatchTreeDeletionValueElement, int, PatchPositions> dvl;
    PatchTreeDeletionValueElement pe(1);
    dvl.addition(pe);
    dvl.deletion(4);
    PatchTreeDeletionValueElement pe3 (3, PatchPositions(1,2,3,4,5,6,7));
    dvl.addition(pe3);
    PatchTreeDeletionValueElement pe4 = dvl.get_element_at(1, 6);
    PatchTreeDeletionValueElement pe5 = dvl.get_element_at(2, 6);
    ASSERT_TRUE(pe4.get_patch_positions() == pe.get_patch_positions());
    ASSERT_TRUE(pe4.get_patch_positions() != pe5.get_patch_positions());
}

TEST(DVIntervalListTest, Serialization1) {
    DVIntervalList<PatchTreeDeletionValueElement, int, PatchPositions> dvl;
    PatchTreeDeletionValueElement pe(1);
    dvl.addition(pe);
    auto data = dvl.serialize();
    DVIntervalList<PatchTreeDeletionValueElement, int, PatchPositions> dvl2;
    dvl2.deserialize(data.first, data.second);
    PatchTreeDeletionValueElement pe2 = dvl2.get_element_at(0, 1);
    ASSERT_EQ(dvl.size(1), dvl2.size(1));
    ASSERT_EQ(dvl.get_element_at(0,1), dvl2.get_element_at(0,1));
}