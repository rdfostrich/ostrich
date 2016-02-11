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
    PatchElements patch;
    patch.add(PatchElement(Triple("s1", "p1", "o1"), true));
    ASSERT_EQ(0, patchTree->append(patch, 0)) << "Appending a patch with one elements failed";
}

TEST_F(PatchTreeTest, AppendContains) {
    /*PatchElements patch;
    patch.add(PatchElement(Triple("s1", "p1", "o1"), true));
    patchTree->append(patch, 0);

    PatchTreeIterator it = patchTree->iterator(Triple("s1", "p1", "o1"));
    PatchTreeKey* key;
    PatchTreeValue* value;
    ASSERT_EQ(true, it.next(&key, &value)) << "Iterator does not contain an element after append";
    ASSERT_EQ("s1 p1 o1.", key->to_string()) << "Found key is incorrect";
    ASSERT_EQ(true, value->addition) << "Found value is incorrect";
    ASSERT_EQ(0, value->patch_id) << "Found value is incorrect";
    ASSERT_EQ(0, value->patch_position) << "Found value is incorrect";
    ASSERT_EQ(false, value->next) << "Found value is incorrect";
    ASSERT_EQ(false, it.next(&key, &value)) << "Iterator contains another element after a single append";*/
    // TODO
}
