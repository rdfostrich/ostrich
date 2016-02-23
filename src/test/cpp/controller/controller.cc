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
