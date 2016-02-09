#include <gtest/gtest.h>

#include "../../../main/cpp/patch/patch_tree.h"

#define TREEFILE "_test_tree.kch"

// The fixture for testing class PatchTree.
class PatchTreeTest : public ::testing::Test {
protected:

    PatchTree* patchTree;

    PatchTreeTest() : patchTree(NULL) {}

    virtual ~PatchTreeTest() {

    }

    virtual void SetUp() {
        remove(TREEFILE);
        patchTree = new PatchTree(TREEFILE);
    }

    virtual void TearDown() {
        delete patchTree;
        remove(TREEFILE);
    }
};

TEST_F(PatchTreeTest, AppendNew) {
    /*PatchElements* patch = (PatchElements *) malloc(sizeof(PatchElements) * 1);
    PatchElements* fillPatchElements = patch;
    *fillPatchElements = PatchElements(PatchElement(Triple("s1", "p1", "o1"), true), false);

    ASSERT_EQ(patchTree->append(patch, 0), 10) << "Appending a patch with one elements failed";*/
    // TODO
}
