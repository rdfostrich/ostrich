#include <gtest/gtest.h>

#include "../../../main/cpp/patch/patch_tree_manager.h"
#include "../../../main/cpp/snapshot/vector_triple_iterator.h"

#define BASEURI "<http://example.org>"

// The fixture for testing class SnapshotManager.
class PatchTreeManagerTest : public ::testing::Test {
protected:
    PatchTreeManager patchTreeManager;

    PatchTreeManagerTest() : patchTreeManager() {}

    virtual void SetUp() {
    }

    virtual void TearDown() {
        // Delete patch files
        std::map<int, PatchTree*> patches = patchTreeManager.get_patch_trees();
        std::map<int, PatchTree*>::iterator itP = patches.begin();
        while(itP != patches.end()) {
            int id = itP->first;
            std::remove(PATCHTREE_FILENAME(id, "spo").c_str());
            std::remove(PATCHTREE_FILENAME(id, "pos").c_str());
            std::remove(PATCHTREE_FILENAME(id, "pso").c_str());
            std::remove(PATCHTREE_FILENAME(id, "sop").c_str());
            std::remove(PATCHTREE_FILENAME(id, "osp").c_str());
            itP++;
        }
    }
};

TEST_F(PatchTreeManagerTest, ConstructPatch) {
    ASSERT_EQ((PatchTree*) NULL, patchTreeManager.get_patch_tree(0)) << "Patch tree with id 0 should not be present.";
    
    patchTreeManager.construct_next_patch_tree(0);
    
    ASSERT_NE((PatchTree*) NULL, patchTreeManager.get_patch_tree(0)) << "Patch tree with id 0 should not be null.";
}

TEST_F(PatchTreeManagerTest, DetectPatchTrees) {
    std::map<int, PatchTree*> found_patches1 = patchTreeManager.detect_patch_trees();
    ASSERT_EQ(true, found_patches1.empty()) << "No patch trees should be detected";
    
    patchTreeManager.construct_next_patch_tree(0);
    std::map<int, PatchTree*> found_patches2 = patchTreeManager.detect_patch_trees();
    ASSERT_EQ(false, found_patches2.empty()) << "One patch tree should be detected";
    ASSERT_EQ(1, found_patches2.size()) << "One patch tree should be detected";
    
    patchTreeManager.construct_next_patch_tree(1);
    std::map<int, PatchTree*> found_patches3 = patchTreeManager.detect_patch_trees();
    ASSERT_EQ(false, found_patches3.empty()) << "Two patch trees should be detected";
    ASSERT_EQ(2, found_patches3.size()) << "Two patch trees should be detected";
}

TEST_F(PatchTreeManagerTest, GetPatchTreeId) {
    ASSERT_EQ(-1, patchTreeManager.get_patch_tree_id(-1));
    ASSERT_EQ(-1, patchTreeManager.get_patch_tree_id(0));
    ASSERT_EQ(-1, patchTreeManager.get_patch_tree_id(1));
    
    patchTreeManager.construct_next_patch_tree(0);
    ASSERT_EQ(-1, patchTreeManager.get_patch_tree_id(-1));
    ASSERT_EQ(0, patchTreeManager.get_patch_tree_id(0));
    ASSERT_EQ(0, patchTreeManager.get_patch_tree_id(1));
    
    patchTreeManager.construct_next_patch_tree(10);
    ASSERT_EQ(-1, patchTreeManager.get_patch_tree_id(-1));
    ASSERT_EQ(0, patchTreeManager.get_patch_tree_id(0));
    ASSERT_EQ(0, patchTreeManager.get_patch_tree_id(1));
    ASSERT_EQ(0, patchTreeManager.get_patch_tree_id(9));
    ASSERT_EQ(10, patchTreeManager.get_patch_tree_id(10));
    ASSERT_EQ(10, patchTreeManager.get_patch_tree_id(11));
    ASSERT_EQ(10, patchTreeManager.get_patch_tree_id(100));
    
    patchTreeManager.construct_next_patch_tree(100);
    ASSERT_EQ(-1, patchTreeManager.get_patch_tree_id(-1));
    ASSERT_EQ(0, patchTreeManager.get_patch_tree_id(0));
    ASSERT_EQ(0, patchTreeManager.get_patch_tree_id(1));
    ASSERT_EQ(0, patchTreeManager.get_patch_tree_id(9));
    ASSERT_EQ(10, patchTreeManager.get_patch_tree_id(10));
    ASSERT_EQ(10, patchTreeManager.get_patch_tree_id(11));
    ASSERT_EQ(10, patchTreeManager.get_patch_tree_id(99));
    ASSERT_EQ(100, patchTreeManager.get_patch_tree_id(100));
    ASSERT_EQ(100, patchTreeManager.get_patch_tree_id(101));
}

TEST_F(PatchTreeManagerTest, AppendPatch) {
    Patch patch1;
    patch1.add(PatchElement(Triple("s1", "p1", "o1"), true));
    patch1.add(PatchElement(Triple("s2", "p2", "o2"), false));
    patch1.add(PatchElement(Triple("s3", "p3", "o3"), false));
    patch1.add(PatchElement(Triple("s4", "p4", "o4"), true));
    
    Patch patch2;
    patch2.add(PatchElement(Triple("s1", "p1", "o1"), true));
    
    Patch patch3;
    patch3.add(PatchElement(Triple("a", "b", "c"), true));
    
    ASSERT_EQ(true, patchTreeManager.append(patch1, 0));
    ASSERT_EQ(false, patchTreeManager.append(patch1, 0)) << "Append shouldn't allow for double appends";
    ASSERT_EQ(false, patchTreeManager.append(patch2, 0)) << "Append shouldn't allow for double appends, not even partial";
    ASSERT_EQ(true, patchTreeManager.append(patch3, 1));
}

TEST_F(PatchTreeManagerTest, GetPatch) {
    Patch patch1;
    patch1.add(PatchElement(Triple("s1", "p1", "o1"), true));
    patch1.add(PatchElement(Triple("s2", "p2", "o2"), false));
    patch1.add(PatchElement(Triple("s3", "p3", "o3"), false));
    patch1.add(PatchElement(Triple("s4", "p4", "o4"), true));
    
    Patch patch2;
    patch2.add(PatchElement(Triple("a", "b", "c"), true));
    
    Patch patch3;
    patch3.add(PatchElement(Triple("s4", "p4", "o4"), false));
    
    patchTreeManager.append(patch1, 0);
    patchTreeManager.append(patch2, 1);
    patchTreeManager.append(patch3, 2);
    
    ASSERT_EQ("s1 p1 o1. (+)\ns2 p2 o2. (-)\ns3 p3 o3. (-)\ns4 p4 o4. (+)\n", patchTreeManager.get_patch(0).to_string());
    ASSERT_EQ("a b c. (+)\ns1 p1 o1. (+)\ns2 p2 o2. (-)\ns3 p3 o3. (-)\ns4 p4 o4. (+)\n", patchTreeManager.get_patch(1).to_string());
    ASSERT_EQ("a b c. (+)\ns1 p1 o1. (+)\ns2 p2 o2. (-)\ns3 p3 o3. (-)\n", patchTreeManager.get_patch(2).to_string());
}
