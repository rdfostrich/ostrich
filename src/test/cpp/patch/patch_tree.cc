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
    PatchElements patch;
    patch.add(PatchElement(Triple("s1", "p1", "o1"), true));
    patchTree->append(patch, 0);

    PatchTreeKey iteratorKey = Triple("s1", "p1", "o1");
    PatchTreeIterator it = patchTree->iterator(&iteratorKey);
    PatchTreeKey key;
    PatchTreeValue value;

    ASSERT_EQ(true, it.next(&key, &value)) << "Iterator does not contain an element after append";
    ASSERT_EQ("s1 p1 o1.", key.to_string()) << "Found key is incorrect";
    ASSERT_EQ(true, value.get(0).is_addition()) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(0).get_patch_id()) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(0).get_patch_position()) << "Found value is incorrect";

    ASSERT_EQ(false, it.next(&key, &value)) << "Iterator contains another element after a single append";
}

TEST_F(PatchTreeTest, IteratorOrder) {
    PatchElements patch;
    patch.add(PatchElement(Triple("g", "p", "o"), false));
    patch.add(PatchElement(Triple("a", "p", "o"), true));
    patch.add(PatchElement(Triple("s", "z", "o"), false));
    patch.add(PatchElement(Triple("s", "a", "o"), true));
    patchTree->append(patch, 0);

    PatchTreeKey iteratorKey = Triple("a", "a", "a");
    PatchTreeIterator it = patchTree->iterator(&iteratorKey);
    PatchTreeKey key;
    PatchTreeValue value;

    ASSERT_EQ(true, it.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("a p o.", key.to_string()) << "First key is incorrect";
    ASSERT_EQ(true, value.get(0).is_addition()) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(0).get_patch_id()) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(0).get_patch_position()) << "Found value is incorrect";

    ASSERT_EQ(true, it.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("g p o.", key.to_string()) << "Second key is incorrect";
    ASSERT_EQ(false, value.get(0).is_addition()) << "Second value is incorrect";
    ASSERT_EQ(0, value.get(0).get_patch_id()) << "Second value is incorrect";
    ASSERT_EQ(0, value.get(0).get_patch_position()) << "Second value is incorrect";

    ASSERT_EQ(true, it.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", key.to_string()) << "Third key is incorrect";
    ASSERT_EQ(true, value.get(0).is_addition()) << "Third value is incorrect";
    ASSERT_EQ(0, value.get(0).get_patch_id()) << "Third value is incorrect";
    ASSERT_EQ(0, value.get(0).get_patch_position()) << "Third value is incorrect";

    ASSERT_EQ(true, it.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("s z o.", key.to_string()) << "Fourth key is incorrect";
    ASSERT_EQ(false, value.get(0).is_addition()) << "Fourth value is incorrect";
    ASSERT_EQ(0, value.get(0).get_patch_id()) << "Fourth value is incorrect";
    ASSERT_EQ(0, value.get(0).get_patch_position()) << "Fourth value is incorrect";

    ASSERT_EQ(false, it.next(&key, &value)) << "Iterator should be finished";
}