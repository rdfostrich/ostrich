#include <gtest/gtest.h>

#include "../../../main/cpp/controller/controller.h"

#define METAFILE "_test_patchtree_meta.bin"

// The fixture for testing class Controller.
class ControllerTest : public ::testing::Test {
protected:
    Controller controller;

    ControllerTest() : controller() {}

    virtual void SetUp() {
        controller = Controller();
    }

    virtual void TearDown() {
        std::map<int, PatchTree*> patches = controller.get_patch_trees();
        std::map<int, PatchTree*>::iterator it = patches.begin();
        while(it != patches.end()) {
            int id = it->first;
            std::remove(PATCHTREE_FILENAME(id).c_str());
            it++;
        }
    }
};

TEST_F(ControllerTest, ConstructPatch) {
    ASSERT_EQ((PatchTree*) NULL, controller.get_patch_tree(0)) << "Patch tree with id 0 should not be present.";

    controller.construct_next_patch_tree(0);

    ASSERT_NE((PatchTree*) NULL, controller.get_patch_tree(0)) << "Patch tree with id 0 should not be null.";
}

TEST_F(ControllerTest, DetectPatchTrees) {
    std::map<int, PatchTree*> found_patches1 = controller.detect_patch_trees();
    ASSERT_EQ(true, found_patches1.empty()) << "No patch trees should be detected";

    controller.construct_next_patch_tree(0);
    std::map<int, PatchTree*> found_patches2 = controller.detect_patch_trees();
    ASSERT_EQ(false, found_patches2.empty()) << "One patch tree should be detected";
    ASSERT_EQ(1, found_patches2.size()) << "One patch tree should be detected";

    controller.construct_next_patch_tree(1);
    std::map<int, PatchTree*> found_patches3 = controller.detect_patch_trees();
    ASSERT_EQ(false, found_patches3.empty()) << "Two patch trees should be detected";
    ASSERT_EQ(2, found_patches3.size()) << "Two patch trees should be detected";
}
