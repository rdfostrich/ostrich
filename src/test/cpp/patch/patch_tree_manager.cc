#include <gtest/gtest.h>

#include "../../../main/cpp/patch/patch_tree_manager.h"
#include "../../../main/cpp/snapshot/vector_triple_iterator.h"

#define BASEURI "<http://example.org>"
#define TESTPATH "./"

// The fixture for testing class SnapshotManager.
class PatchTreeManagerTest : public ::testing::Test {
protected:
    PatchTreeManager* patchTreeManager;
    DictionaryManager dict;

    PatchTreeManagerTest() : patchTreeManager(), dict(DictionaryManager(TESTPATH, 0)) {}

    virtual void SetUp() {
        patchTreeManager = new PatchTreeManager(TESTPATH);
    }

    virtual void TearDown() {
        // Delete patch files
        std::map<int, PatchTree*> patches = patchTreeManager->get_patch_trees();
        std::map<int, PatchTree*>::iterator itP = patches.begin();
        std::list<int> patchMetadataToDelete;
        while(itP != patches.end()) {
            int id = itP->first;
            std::remove((TESTPATH + PATCHTREE_FILENAME(id, "spo_deletions")).c_str());
            std::remove((TESTPATH + PATCHTREE_FILENAME(id, "spo")).c_str());
            std::remove((TESTPATH + PATCHTREE_FILENAME(id, "pos")).c_str());
            std::remove((TESTPATH + PATCHTREE_FILENAME(id, "pso")).c_str());
            std::remove((TESTPATH + PATCHTREE_FILENAME(id, "sop")).c_str());
            std::remove((TESTPATH + PATCHTREE_FILENAME(id, "osp")).c_str());
            patchMetadataToDelete.push_back(id);
            itP++;
        }
        delete patchTreeManager;

        DictionaryManager::cleanup(TESTPATH, 0);

        // Delete metadata files
        std::list<int>::iterator it2;
        for(it2=patchMetadataToDelete.begin(); it2!=patchMetadataToDelete.end(); ++it2) {
            std::remove((TESTPATH + METADATA_FILENAME_BASE(*it2)).c_str());
        }
    }
};

TEST_F(PatchTreeManagerTest, ConstructPatch) {
    ASSERT_EQ((PatchTree*) NULL, patchTreeManager->get_patch_tree(0, &dict)) << "Patch tree with id 0 should not be present.";
    
    patchTreeManager->construct_next_patch_tree(0, &dict);
    
    ASSERT_NE((PatchTree*) NULL, patchTreeManager->get_patch_tree(0, &dict)) << "Patch tree with id 0 should not be null.";
}

TEST_F(PatchTreeManagerTest, DetectPatchTrees) {
    std::map<int, PatchTree*> found_patches1 = patchTreeManager->detect_patch_trees();
    ASSERT_EQ(true, found_patches1.empty()) << "No patch trees should be detected";
    
    patchTreeManager->construct_next_patch_tree(0, &dict);
    std::map<int, PatchTree*> found_patches2 = patchTreeManager->detect_patch_trees();
    ASSERT_EQ(false, found_patches2.empty()) << "One patch tree should be detected";
    ASSERT_EQ(1, found_patches2.size()) << "One patch tree should be detected";
    
    patchTreeManager->construct_next_patch_tree(1, &dict);
    std::map<int, PatchTree*> found_patches3 = patchTreeManager->detect_patch_trees();
    ASSERT_EQ(false, found_patches3.empty()) << "Two patch trees should be detected";
    ASSERT_EQ(2, found_patches3.size()) << "Two patch trees should be detected";
}

TEST_F(PatchTreeManagerTest, GetPatchTreeId) {
    ASSERT_EQ(-1, patchTreeManager->get_patch_tree_id(-1));
    ASSERT_EQ(-1, patchTreeManager->get_patch_tree_id(0));
    ASSERT_EQ(-1, patchTreeManager->get_patch_tree_id(1));
    
    patchTreeManager->construct_next_patch_tree(0, &dict);
    ASSERT_EQ(-1, patchTreeManager->get_patch_tree_id(-1));
    ASSERT_EQ(0, patchTreeManager->get_patch_tree_id(0));
    ASSERT_EQ(0, patchTreeManager->get_patch_tree_id(1));
    
    patchTreeManager->construct_next_patch_tree(10, &dict);
    ASSERT_EQ(-1, patchTreeManager->get_patch_tree_id(-1));
    ASSERT_EQ(0, patchTreeManager->get_patch_tree_id(0));
    ASSERT_EQ(0, patchTreeManager->get_patch_tree_id(1));
    ASSERT_EQ(0, patchTreeManager->get_patch_tree_id(9));
    ASSERT_EQ(10, patchTreeManager->get_patch_tree_id(10));
    ASSERT_EQ(10, patchTreeManager->get_patch_tree_id(11));
    ASSERT_EQ(10, patchTreeManager->get_patch_tree_id(100));
    
    patchTreeManager->construct_next_patch_tree(100, &dict);
    ASSERT_EQ(-1, patchTreeManager->get_patch_tree_id(-1));
    ASSERT_EQ(0, patchTreeManager->get_patch_tree_id(0));
    ASSERT_EQ(0, patchTreeManager->get_patch_tree_id(1));
    ASSERT_EQ(0, patchTreeManager->get_patch_tree_id(9));
    ASSERT_EQ(10, patchTreeManager->get_patch_tree_id(10));
    ASSERT_EQ(10, patchTreeManager->get_patch_tree_id(11));
    ASSERT_EQ(10, patchTreeManager->get_patch_tree_id(99));
    ASSERT_EQ(100, patchTreeManager->get_patch_tree_id(100));
    ASSERT_EQ(100, patchTreeManager->get_patch_tree_id(101));
}

TEST_F(PatchTreeManagerTest, AppendPatch) {
    Patch patch1(&dict);
    patch1.add(PatchElement(Triple("s1", "p1", "o1", &dict), true));
    patch1.add(PatchElement(Triple("s2", "p2", "o2", &dict), false));
    patch1.add(PatchElement(Triple("s3", "p3", "o3", &dict), false));
    patch1.add(PatchElement(Triple("s4", "p4", "o4", &dict), true));
    
    Patch patch2(&dict);
    patch2.add(PatchElement(Triple("s1", "p1", "o1", &dict), true));
    
    Patch patch3(&dict);
    patch3.add(PatchElement(Triple("a", "b", "c", &dict), true));
    
    ASSERT_EQ(true, patchTreeManager->append(patch1, 0, &dict));
    ASSERT_EQ(false, patchTreeManager->append(patch1, 0, &dict)) << "Append shouldn't allow for double appends";
    ASSERT_EQ(false, patchTreeManager->append(patch2, 0, &dict)) << "Append shouldn't allow for double appends, not even partial";
    ASSERT_EQ(true, patchTreeManager->append(patch3, 1, &dict));
}

TEST_F(PatchTreeManagerTest, GetPatch) {
    Patch patch1(&dict);
    patch1.add(PatchElement(Triple("s1", "p1", "o1", &dict), true));
    patch1.add(PatchElement(Triple("s2", "p2", "o2", &dict), false));
    patch1.add(PatchElement(Triple("s3", "p3", "o3", &dict), false));
    patch1.add(PatchElement(Triple("s4", "p4", "o4", &dict), true));
    
    Patch patch2(&dict);
    patch2.add(PatchElement(Triple("a", "b", "c", &dict), true));
    
    Patch patch3(&dict);
    patch3.add(PatchElement(Triple("s4", "p4", "o4", &dict), false));
    
    patchTreeManager->append(patch1, 0, &dict);
    patchTreeManager->append(patch2, 1, &dict);
    patchTreeManager->append(patch3, 2, &dict);
    
    ASSERT_EQ("s1 p1 o1. (+)\ns2 p2 o2. (-)\ns3 p3 o3. (-)\ns4 p4 o4. (+)\n", patchTreeManager->get_patch(0, &dict).to_string(dict));
    ASSERT_EQ("a b c. (+)\ns1 p1 o1. (+)\ns2 p2 o2. (-)\ns3 p3 o3. (-)\ns4 p4 o4. (+)\n", patchTreeManager->get_patch(1, &dict).to_string(dict));
    ASSERT_EQ("a b c. (+)\ns1 p1 o1. (+)\ns2 p2 o2. (-)\ns3 p3 o3. (-)\n", patchTreeManager->get_patch(2, &dict).to_string(dict));
}
