#include <gtest/gtest.h>

#include "../../../main/cpp/patch/patch_tree.h"

TEST(PatchTreeValueTest, Fields) {
    PatchTreeValue value;
    value.add(PatchTreeValueElement(0, 1, true));
    ASSERT_EQ(0, value.get(0).get_patch_id()) << "Patch id is not saved correctly";
    ASSERT_EQ(1, value.get(0).get_patch_position()) << "Patch position is not saved correctly";
    ASSERT_EQ(true, value.get(0).is_addition()) << "Patch addition is not saved correctly";
}

TEST(PatchTreeValueTest, ToString) {
    PatchTreeValue value;
    value.add(PatchTreeValueElement(0, 1, true));
    value.add(PatchTreeValueElement(1, 82, false));
    value.add(PatchTreeValueElement(20, 3, false));
    value.add(PatchTreeValueElement(10, 742, true));
    ASSERT_EQ("{0:1(+),1:82(-),10:742(+),20:3(-)}", value.to_string()) << "to_string is incorrect";
}

TEST(PatchTreeValueTest, Lookup) {
    PatchTreeValue value;
    value.add(PatchTreeValueElement(0, 1, true));
    value.add(PatchTreeValueElement(1, 82, false));
    value.add(PatchTreeValueElement(20, 3, false));
    value.add(PatchTreeValueElement(10, 742, true));

    ASSERT_EQ(0, value.get(0).get_patch_id()) << "Patch element 0 is incorrect";
    ASSERT_EQ(1, value.get(0).get_patch_position()) << "Patch element 0 is incorrect";
    ASSERT_EQ(true, value.get(0).is_addition()) << "Patch element 0 is incorrect";

    ASSERT_EQ(1, value.get(1).get_patch_id()) << "Patch element 1 is incorrect";
    ASSERT_EQ(82, value.get(1).get_patch_position()) << "Patch element 1 is incorrect";
    ASSERT_EQ(false, value.get(1).is_addition()) << "Patch element 1 is incorrect";

    ASSERT_EQ(20, value.get(20).get_patch_id()) << "Patch element 20 is incorrect";
    ASSERT_EQ(3, value.get(20).get_patch_position()) << "Patch element 20 is incorrect";
    ASSERT_EQ(false, value.get(20).is_addition()) << "Patch element 20 is incorrect";

    ASSERT_EQ(10, value.get(10).get_patch_id()) << "Patch element 10 is incorrect";
    ASSERT_EQ(742, value.get(10).get_patch_position()) << "Patch element 10 is incorrect";
    ASSERT_EQ(true, value.get(10).is_addition()) << "Patch element 10 is incorrect";
}

TEST(PatchTreeValueTest, Serialization) {
    PatchTreeValue valueIn;
    valueIn.add(PatchTreeValueElement(0, 1, true));
    valueIn.add(PatchTreeValueElement(1, 82, false));
    valueIn.add(PatchTreeValueElement(20, 3, false));
    valueIn.add(PatchTreeValueElement(10, 742, true));

    // Serialize
    size_t size;
    const char* data = valueIn.serialize(&size);

    // Deserialize
    PatchTreeValue valueOut;
    valueOut.deserialize(data, size);

    ASSERT_EQ(valueIn.to_string(), valueOut.to_string()) << "Serialization failed";
    free((char*) data);
}

TEST(PatchTreeValueTest, SerializationSize) {
    PatchTreeValue valueIn;
    valueIn.add(PatchTreeValueElement(0, 1, true));
    valueIn.add(PatchTreeValueElement(1, 82, false));
    valueIn.add(PatchTreeValueElement(20, 3, false));
    valueIn.add(PatchTreeValueElement(10, 742, true));

    // Serialize
    size_t size;
    const char* data = valueIn.serialize(&size);

    ASSERT_EQ(sizeof(PatchTreeValueElement) * 4, size) << "Serialization length is too high";
    free((char*) data);
}