#include <gtest/gtest.h>

#include "../../../main/cpp/patch/patch_tree.h"

TEST(PatchTreeValueTest, Empty) {
    PatchTreeValue value;

    ASSERT_EQ(false, value.is_addition(0, true)) << "An non-initialized value must not be an addition";
    ASSERT_EQ(false, value.is_addition(1, true)) << "An non-initialized value must not be an addition";
    ASSERT_EQ(false, value.is_addition(0, false)) << "An non-initialized value must not be an addition";
    ASSERT_EQ(false, value.is_addition(1, false)) << "An non-initialized value must not be an addition";

    ASSERT_EQ(false, value.is_deletion(0, true)) << "An non-initialized value must not be a deletion";
    ASSERT_EQ(false, value.is_deletion(1, true)) << "An non-initialized value must not be a deletion";
    ASSERT_EQ(false, value.is_deletion(0, false)) << "An non-initialized value must not be a deletion";
    ASSERT_EQ(false, value.is_deletion(1, false)) << "An non-initialized value must not be a deletion";

    ASSERT_EQ(false, value.is_local_change(0)) << "An non-initialized value must not be a local change";
    ASSERT_EQ(false, value.is_local_change(1)) << "An non-initialized value must not be a local change";
}

TEST(PatchTreeValueTest, Addition1) {
    PatchTreeValue value;

    value.set_addition(true);
    value.get_addition()->add(0);
    value.get_addition()->set_local_change(0);

    ASSERT_EQ(true, value.is_addition(0, true)) << "An non-initialized value must not be an addition";
    ASSERT_EQ(false, value.is_addition(1, true)) << "An non-initialized value must not be an addition";
    ASSERT_EQ(true, value.is_addition(0, false)) << "An non-initialized value must not be an addition";
    ASSERT_EQ(true, value.is_addition(1, false)) << "An non-initialized value must not be an addition";

    ASSERT_EQ(false, value.is_deletion(0, true)) << "An non-initialized value must not be a deletion";
    ASSERT_EQ(false, value.is_deletion(1, true)) << "An non-initialized value must not be a deletion";
    ASSERT_EQ(false, value.is_deletion(0, false)) << "An non-initialized value must not be a deletion";
    ASSERT_EQ(false, value.is_deletion(1, false)) << "An non-initialized value must not be a deletion";

    ASSERT_EQ(true, value.is_local_change(0)) << "Set value must be a local change";
    ASSERT_EQ(true, value.is_local_change(1)) << "Following patch ids must be a local change as well even if not explicitly set";
}

TEST(PatchTreeValueTest, Addition2) {
    PatchTreeValue value;

    value.set_addition(true);
    value.get_addition()->add(1);
    value.get_addition()->set_local_change(1);

    ASSERT_EQ(false, value.is_addition(0, true)) << "An non-initialized value must not be an addition";
    ASSERT_EQ(true, value.is_addition(1, true)) << "An non-initialized value must not be an addition";
    ASSERT_EQ(false, value.is_addition(0, false)) << "An non-initialized value must not be an addition";
    ASSERT_EQ(true, value.is_addition(1, true)) << "An non-initialized value must not be an addition";

    ASSERT_EQ(false, value.is_deletion(0, true)) << "An non-initialized value must not be a deletion";
    ASSERT_EQ(false, value.is_deletion(1, true)) << "An non-initialized value must not be a deletion";
    ASSERT_EQ(false, value.is_deletion(0, false)) << "An non-initialized value must not be a deletion";
    ASSERT_EQ(false, value.is_deletion(1, false)) << "An non-initialized value must not be a deletion";

    ASSERT_EQ(false, value.is_local_change(0)) << "An non-initialized value must not be a local change";
    ASSERT_EQ(true, value.is_local_change(1)) << "An non-initialized value must not be a local change";
}
