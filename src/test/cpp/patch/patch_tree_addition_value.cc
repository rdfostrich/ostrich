#include <gtest/gtest.h>

#include "../../../main/cpp/patch/patch_tree_addition_value.h"

#ifdef COMPRESSED_ADD_VALUES
TEST(PatchTreeAdditionValueTest, ToString) {
    PatchTreeAdditionValue value(10);
    value.add_unique(0);
    value.add_unique(10);
    value.add_unique(5);
    value.add_unique(2);
    value.add_unique(7);
    value.add_unique(7);
    value.add_unique(7);

    value.set_local_change_unique(5);
    value.set_local_change_unique(10);

    ASSERT_EQ("{0,2,5L,7,10L}", value.to_string()) << "to_string is incorrect";
}
#else
TEST(PatchTreeAdditionValueTest, ToString) {
    PatchTreeAdditionValue value;
    value.add(0);
    value.add(10);
    value.add(5);
    value.add(2);
    value.add(7);
    value.add(7);
    value.add(7);

    value.set_local_change(5);
    value.set_local_change(10);

    ASSERT_EQ("{0,2,5L,7,10L}", value.to_string()) << "to_string is incorrect";
}
#endif

#ifdef COMPRESSED_ADD_VALUES
TEST(PatchTreeAdditionValueTest, Lookup) {
    PatchTreeAdditionValue value(10);
    value.add_unique(0);
    value.add_unique(10);
    value.add_unique(5);
    value.add_unique(2);
    value.add_unique(7);
    value.add_unique(7);
    value.add_unique(7);

    value.set_local_change_unique(5);

    ASSERT_EQ(true, value.is_patch_id(0)) << "to_string is incorrect";
    ASSERT_EQ(false, value.is_patch_id(1)) << "to_string is incorrect";
    ASSERT_EQ(true, value.is_patch_id(2)) << "to_string is incorrect";
    ASSERT_EQ(false, value.is_patch_id(3)) << "to_string is incorrect";
    ASSERT_EQ(false, value.is_patch_id(4)) << "to_string is incorrect";
    ASSERT_EQ(true, value.is_patch_id(5)) << "to_string is incorrect";
    ASSERT_EQ(false, value.is_patch_id(6)) << "to_string is incorrect";
    ASSERT_EQ(true, value.is_patch_id(7)) << "to_string is incorrect";
    ASSERT_EQ(false, value.is_patch_id(8)) << "to_string is incorrect";
    ASSERT_EQ(false, value.is_patch_id(9)) << "to_string is incorrect";
    ASSERT_EQ(true, value.is_patch_id(10)) << "to_string is incorrect";
    ASSERT_EQ(false, value.is_patch_id(11)) << "to_string is incorrect";

    ASSERT_EQ(false, value.is_local_change(4)) << "to_string is incorrect";
    ASSERT_EQ(true, value.is_local_change(5)) << "to_string is incorrect";
    ASSERT_EQ(false, value.is_local_change(6)) << "to_string is incorrect";
}
#else
TEST(PatchTreeAdditionValueTest, Lookup) {
    PatchTreeAdditionValue value;
    value.add(0);
    value.add(10);
    value.add(5);
    value.add(2);
    value.add(7);
    value.add(7);
    value.add(7);

    value.set_local_change(5);

    ASSERT_EQ(true, value.is_patch_id(0)) << "to_string is incorrect";
    ASSERT_EQ(false, value.is_patch_id(1)) << "to_string is incorrect";
    ASSERT_EQ(true, value.is_patch_id(2)) << "to_string is incorrect";
    ASSERT_EQ(false, value.is_patch_id(3)) << "to_string is incorrect";
    ASSERT_EQ(false, value.is_patch_id(4)) << "to_string is incorrect";
    ASSERT_EQ(true, value.is_patch_id(5)) << "to_string is incorrect";
    ASSERT_EQ(false, value.is_patch_id(6)) << "to_string is incorrect";
    ASSERT_EQ(true, value.is_patch_id(7)) << "to_string is incorrect";
    ASSERT_EQ(false, value.is_patch_id(8)) << "to_string is incorrect";
    ASSERT_EQ(false, value.is_patch_id(9)) << "to_string is incorrect";
    ASSERT_EQ(true, value.is_patch_id(10)) << "to_string is incorrect";
    ASSERT_EQ(false, value.is_patch_id(11)) << "to_string is incorrect";

    ASSERT_EQ(false, value.is_local_change(4)) << "to_string is incorrect";
    ASSERT_EQ(true, value.is_local_change(5)) << "to_string is incorrect";
    ASSERT_EQ(true, value.is_local_change(6)) << "to_string is incorrect";
}
#endif

#ifdef COMPRESSED_ADD_VALUES
TEST(PatchTreeAdditionValueTest, Serialization) {
    PatchTreeAdditionValue valueIn(10);
    valueIn.add_unique(0);
    valueIn.add_unique(10);
    valueIn.add_unique(5);
    valueIn.add_unique(2);
    valueIn.add_unique(7);
    valueIn.add_unique(7);
    valueIn.add_unique(7);

    valueIn.set_local_change_unique(5);

    // Serialize
    size_t size;
    const char* data = valueIn.serialize(&size);

    // Deserialize
    PatchTreeAdditionValue valueOut(10);
    valueOut.deserialize(data, size);

    ASSERT_EQ(valueIn.to_string(), valueOut.to_string()) << "Serialization failed";
    delete[] data;
}
#else
TEST(PatchTreeAdditionValueTest, Serialization) {
    PatchTreeAdditionValue valueIn;
    valueIn.add(0);
    valueIn.add(10);
    valueIn.add(5);
    valueIn.add(2);
    valueIn.add(7);
    valueIn.add(7);
    valueIn.add(7);

    valueIn.set_local_change(5);

    // Serialize
    size_t size;
    const char* data = valueIn.serialize(&size);

    // Deserialize
    PatchTreeAdditionValue valueOut;
    valueOut.deserialize(data, size);

    ASSERT_EQ(valueIn.to_string(), valueOut.to_string()) << "Serialization failed";
    free((char*) data);
}
#endif