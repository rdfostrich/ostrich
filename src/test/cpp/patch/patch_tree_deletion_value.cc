#include <gtest/gtest.h>

#include "../../../main/cpp/patch/patch_tree.h"


#ifdef COMPRESSED_DEL_VALUES

TEST(PatchTreeDeletionValueTest, Fields) {
    PatchTreeDeletionValue value(7);
    value.add(PatchTreeDeletionValueElement(0, PatchPositions(1, 2, 3, 4, 5, 6, 7)));
    ASSERT_EQ(0, value.get(0).get_patch_id()) << "Patch id is not saved correctly";
    ASSERT_EQ(1, value.get(0).get_patch_positions().sp_) << "Patch position is not saved correctly";
    ASSERT_EQ(2, value.get(0).get_patch_positions().s_o) << "Patch position is not saved correctly";
    ASSERT_EQ(3, value.get(0).get_patch_positions().s__) << "Patch position is not saved correctly";
    ASSERT_EQ(4, value.get(0).get_patch_positions()._po) << "Patch position is not saved correctly";
    ASSERT_EQ(5, value.get(0).get_patch_positions()._p_) << "Patch position is not saved correctly";
    ASSERT_EQ(6, value.get(0).get_patch_positions().__o) << "Patch position is not saved correctly";
    ASSERT_EQ(7, value.get(0).get_patch_positions().___) << "Patch position is not saved correctly";
}

TEST(PatchTreeDeletionValueTest, ToString) {
    PatchTreeDeletionValue value(20);
    value.add(PatchTreeDeletionValueElement(0, PatchPositions(1, 2, 3, 4, 5, 6, 7)));
    value.add(PatchTreeDeletionValueElement(1, PatchPositions(82, 83, 84, 85, 86, 87, 88)));
    value.del(2);
    value.add(PatchTreeDeletionValueElement(10, PatchPositions(742, 743, 744, 745, 746, 747, 748)));
    value.del(11);
    value.add(PatchTreeDeletionValueElement(20, PatchPositions(3, 4, 5, 6, 7, 8, 9)));
    value.del(21);
    ASSERT_EQ("{0:{ 1 2 3 4 5 6 7 },1:{ 82 83 84 85 86 87 88 },10:{ 742 743 744 745 746 747 748 },20:{ 3 4 5 6 7 8 9 }}", value.to_string()) << "to_string is incorrect";
}

TEST(PatchTreeDeletionValueTest, Lookup) {
    PatchTreeDeletionValue value(20);
    value.add(PatchTreeDeletionValueElement(0, PatchPositions(1, 2, 3, 4, 5, 6, 7)));
    value.add(PatchTreeDeletionValueElement(1, PatchPositions(82, 83, 84, 85, 86, 87, 88)));
    value.add(PatchTreeDeletionValueElement(10, PatchPositions(742, 743, 744, 745, 746, 747, 748)));
    value.add(PatchTreeDeletionValueElement(20, PatchPositions(3, 4, 5, 6, 7, 8, 9)));

    ASSERT_EQ(0, value.get(0).get_patch_id()) << "Patch element 0 is incorrect";
    ASSERT_EQ(1, value.get(0).get_patch_positions().sp_) << "Patch element 0 is incorrect";

    ASSERT_EQ(1, value.get(1).get_patch_id()) << "Patch element 1 is incorrect";
    ASSERT_EQ(82, value.get(1).get_patch_positions().sp_) << "Patch element 1 is incorrect";

    ASSERT_EQ(20, value.get(20).get_patch_id()) << "Patch element 20 is incorrect";
    ASSERT_EQ(3, value.get(20).get_patch_positions().sp_) << "Patch element 20 is incorrect";

    ASSERT_EQ(10, value.get(10).get_patch_id()) << "Patch element 10 is incorrect";
    ASSERT_EQ(742, value.get(10).get_patch_positions().sp_) << "Patch element 10 is incorrect";
}

TEST(PatchTreeDeletionValueTest, Serialization) {
    PatchTreeDeletionValue valueIn(20);
    valueIn.add(PatchTreeDeletionValueElement(0, PatchPositions(1, 2, 3, 4, 5, 6, 7)));
    valueIn.add(PatchTreeDeletionValueElement(1, PatchPositions(82, 83, 84, 85, 86, 87, 88)));
    valueIn.add(PatchTreeDeletionValueElement(10, PatchPositions(742, 743, 744, 745, 746, 747, 748)));
    valueIn.add(PatchTreeDeletionValueElement(20, PatchPositions(3, 4, 5, 6, 7, 8, 9)));

    // Serialize
    size_t size;
    const char* data = valueIn.serialize(&size);

    // Deserialize
    PatchTreeDeletionValue valueOut(20);
    valueOut.deserialize(data, size);

    ASSERT_EQ(valueIn.to_string(), valueOut.to_string()) << "Serialization failed";
    delete[] data;
}

TEST(PatchTreeDeletionValueTest, SerializationComplex) {
    int max = 10;
    PatchTreeDeletionValue original_value(max);
    for (int i=0; i<max; i++) {
        original_value.add(PatchTreeDeletionValueElement(i, PatchPositions(i*100,i*100,i*100,i*100,i*100,i*100,i*100)));
    }
    size_t data_size;
    const char* data = original_value.serialize(&data_size);
    PatchTreeDeletionValue new_value(max);
    new_value.deserialize(data, data_size);
    for (int i=0; i<max; i++) {
        PatchTreeDeletionValueElement pe = new_value.get_patch_at(i);
        ASSERT_EQ(i, pe.get_patch_id()) << "The patch ID is incorrect";
        ASSERT_EQ(PatchPositions(i*100,i*100,i*100,i*100,i*100,i*100,i*100).to_string(), pe.get_patch_positions().to_string()) << "The patch positions are incorrect";
    }
}

#else

TEST(PatchTreeDeletionValueTest, Fields) {
    PatchTreeDeletionValue value;
    value.add(PatchTreeDeletionValueElement(0, PatchPositions(1, 2, 3, 4, 5, 6, 7)));
    ASSERT_EQ(0, value.get(0).get_patch_id()) << "Patch id is not saved correctly";
    ASSERT_EQ(1, value.get(0).get_patch_positions().sp_) << "Patch position is not saved correctly";
    ASSERT_EQ(2, value.get(0).get_patch_positions().s_o) << "Patch position is not saved correctly";
    ASSERT_EQ(3, value.get(0).get_patch_positions().s__) << "Patch position is not saved correctly";
    ASSERT_EQ(4, value.get(0).get_patch_positions()._po) << "Patch position is not saved correctly";
    ASSERT_EQ(5, value.get(0).get_patch_positions()._p_) << "Patch position is not saved correctly";
    ASSERT_EQ(6, value.get(0).get_patch_positions().__o) << "Patch position is not saved correctly";
    ASSERT_EQ(7, value.get(0).get_patch_positions().___) << "Patch position is not saved correctly";
}

TEST(PatchTreeDeletionValueTest, ToString) {
    PatchTreeDeletionValue value;
    value.add(PatchTreeDeletionValueElement(0, PatchPositions(1, 2, 3, 4, 5, 6, 7)));
    value.add(PatchTreeDeletionValueElement(1, PatchPositions(82, 83, 84, 85, 86, 87, 88)));
    value.add(PatchTreeDeletionValueElement(20, PatchPositions(3, 4, 5, 6, 7, 8, 9)));
    value.add(PatchTreeDeletionValueElement(10, PatchPositions(742, 743, 744, 745, 746, 747, 748)));
    ASSERT_EQ("{0:{ 1 2 3 4 5 6 7 },1:{ 82 83 84 85 86 87 88 },10:{ 742 743 744 745 746 747 748 },20:{ 3 4 5 6 7 8 9 }}", value.to_string()) << "to_string is incorrect";
}

TEST(PatchTreeDeletionValueTest, Lookup) {
    PatchTreeDeletionValue value;
    value.add(PatchTreeDeletionValueElement(0, PatchPositions(1, 2, 3, 4, 5, 6, 7)));
    value.add(PatchTreeDeletionValueElement(1, PatchPositions(82, 83, 84, 85, 86, 87, 88)));
    value.add(PatchTreeDeletionValueElement(20, PatchPositions(3, 4, 5, 6, 7, 8, 9)));
    value.add(PatchTreeDeletionValueElement(10, PatchPositions(742, 743, 744, 745, 746, 747, 748)));

    ASSERT_EQ(0, value.get(0).get_patch_id()) << "Patch element 0 is incorrect";
    ASSERT_EQ(1, value.get(0).get_patch_positions().sp_) << "Patch element 0 is incorrect";

    ASSERT_EQ(1, value.get(1).get_patch_id()) << "Patch element 1 is incorrect";
    ASSERT_EQ(82, value.get(1).get_patch_positions().sp_) << "Patch element 1 is incorrect";

    ASSERT_EQ(20, value.get(20).get_patch_id()) << "Patch element 20 is incorrect";
    ASSERT_EQ(3, value.get(20).get_patch_positions().sp_) << "Patch element 20 is incorrect";

    ASSERT_EQ(10, value.get(10).get_patch_id()) << "Patch element 10 is incorrect";
    ASSERT_EQ(742, value.get(10).get_patch_positions().sp_) << "Patch element 10 is incorrect";
}

TEST(PatchTreeDeletionValueTest, Serialization) {
    PatchTreeDeletionValue valueIn;
    valueIn.add(PatchTreeDeletionValueElement(0, PatchPositions(1, 2, 3, 4, 5, 6, 7)));
    valueIn.add(PatchTreeDeletionValueElement(1, PatchPositions(82, 83, 84, 85, 86, 87, 88)));
    valueIn.add(PatchTreeDeletionValueElement(20, PatchPositions(3, 4, 5, 6, 7, 8, 9)));
    valueIn.add(PatchTreeDeletionValueElement(10, PatchPositions(742, 743, 744, 745, 746, 747, 748)));

    // Serialize
    size_t size;
    const char* data = valueIn.serialize(&size);

    // Deserialize
    PatchTreeDeletionValue valueOut;
    valueOut.deserialize(data, size);

    ASSERT_EQ(valueIn.to_string(), valueOut.to_string()) << "Serialization failed";
    delete[] data;
}

//TEST(PatchTreeDeletionValueTest, SerializationSize) {
//    PatchTreeDeletionValue valueIn;
//    valueIn.add(PatchTreeDeletionValueElement(0, PatchPositions(1, 2, 3, 4, 5, 6, 7)));
//    valueIn.add(PatchTreeDeletionValueElement(1, PatchPositions(82, 83, 84, 85, 86, 87, 88)));
//    valueIn.add(PatchTreeDeletionValueElement(20, PatchPositions(3, 4, 5, 6, 7, 8, 9)));
//    valueIn.add(PatchTreeDeletionValueElement(10, PatchPositions(742, 743, 744, 745, 746, 747, 748)));
//
//    // Serialize
//    size_t size;
//    const char* data = valueIn.serialize(&size);
//
//    ASSERT_EQ(sizeof(PatchTreeDeletionValueElement) * 4, size) << "Serialization length is too high";
//    delete[] data;
//}

#endif
