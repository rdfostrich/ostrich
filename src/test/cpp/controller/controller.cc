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

TEST_F(ControllerTest, GetPatchTreeId) {
    ASSERT_EQ(-1, controller.get_patch_tree_id(-1));
    ASSERT_EQ(-1, controller.get_patch_tree_id(0));
    ASSERT_EQ(-1, controller.get_patch_tree_id(1));

    controller.construct_next_patch_tree(0);
    ASSERT_EQ(-1, controller.get_patch_tree_id(-1));
    ASSERT_EQ(0, controller.get_patch_tree_id(0));
    ASSERT_EQ(0, controller.get_patch_tree_id(1));

    controller.construct_next_patch_tree(10);
    ASSERT_EQ(-1, controller.get_patch_tree_id(-1));
    ASSERT_EQ(0, controller.get_patch_tree_id(0));
    ASSERT_EQ(0, controller.get_patch_tree_id(1));
    ASSERT_EQ(0, controller.get_patch_tree_id(9));
    ASSERT_EQ(10, controller.get_patch_tree_id(10));
    ASSERT_EQ(10, controller.get_patch_tree_id(11));
    ASSERT_EQ(10, controller.get_patch_tree_id(100));

    controller.construct_next_patch_tree(100);
    ASSERT_EQ(-1, controller.get_patch_tree_id(-1));
    ASSERT_EQ(0, controller.get_patch_tree_id(0));
    ASSERT_EQ(0, controller.get_patch_tree_id(1));
    ASSERT_EQ(0, controller.get_patch_tree_id(9));
    ASSERT_EQ(10, controller.get_patch_tree_id(10));
    ASSERT_EQ(10, controller.get_patch_tree_id(11));
    ASSERT_EQ(10, controller.get_patch_tree_id(99));
    ASSERT_EQ(100, controller.get_patch_tree_id(100));
    ASSERT_EQ(100, controller.get_patch_tree_id(101));
}

TEST_F(ControllerTest, AppendPatch) {
    Patch patch1;
    patch1.add(PatchElement(Triple("s1", "p1", "o1"), true));
    patch1.add(PatchElement(Triple("s2", "p2", "o2"), false));
    patch1.add(PatchElement(Triple("s3", "p3", "o3"), false));
    patch1.add(PatchElement(Triple("s4", "p4", "o4"), true));

    Patch patch2;
    patch2.add(PatchElement(Triple("s1", "p1", "o1"), true));

    Patch patch3;
    patch3.add(PatchElement(Triple("a", "b", "c"), true));

    ASSERT_EQ(true, controller.append(patch1, 0));
    ASSERT_EQ(false, controller.append(patch1, 0)) << "Append shouldn't allow for double appends";
    ASSERT_EQ(false, controller.append(patch2, 0)) << "Append shouldn't allow for double appends, not even partial";
    ASSERT_EQ(true, controller.append(patch3, 1));
}

TEST_F(ControllerTest, GetPatch) {
    Patch patch1;
    patch1.add(PatchElement(Triple("s1", "p1", "o1"), true));
    patch1.add(PatchElement(Triple("s2", "p2", "o2"), false));
    patch1.add(PatchElement(Triple("s3", "p3", "o3"), false));
    patch1.add(PatchElement(Triple("s4", "p4", "o4"), true));

    Patch patch2;
    patch2.add(PatchElement(Triple("a", "b", "c"), true));

    Patch patch3;
    patch3.add(PatchElement(Triple("s4", "p4", "o4"), false));

    controller.append(patch1, 0);
    controller.append(patch2, 1);
    controller.append(patch3, 2);

    ASSERT_EQ("s1 p1 o1. (+)\ns2 p2 o2. (-)\ns3 p3 o3. (-)\ns4 p4 o4. (+)\n", controller.get_patch(0).to_string());
    ASSERT_EQ("a b c. (+)\ns1 p1 o1. (+)\ns2 p2 o2. (-)\ns3 p3 o3. (-)\ns4 p4 o4. (+)\n", controller.get_patch(1).to_string());
    ASSERT_EQ("a b c. (+)\ns1 p1 o1. (+)\ns2 p2 o2. (-)\ns3 p3 o3. (-)\n", controller.get_patch(2).to_string());
}
