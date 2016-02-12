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
    Patch patch;
    patch.add(PatchElement(Triple("s1", "p1", "o1"), true));
    ASSERT_EQ(0, patchTree->append(patch, 0)) << "Appending a patch with one elements failed";
}

TEST_F(PatchTreeTest, AppendContains) {
    Patch patch;
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
    Patch patch;
    patch.add(PatchElement(Triple("g", "p", "o"), false));
    patch.add(PatchElement(Triple("a", "p", "o"), true));
    patch.add(PatchElement(Triple("s", "z", "o"), false));
    patch.add(PatchElement(Triple("s", "a", "o"), true));
    patchTree->append(patch, 0);

    PatchTreeKey iteratorKey = Triple("a", "a", "a");
    PatchTreeIterator it = patchTree->iterator(&iteratorKey); // Iterate starting from the given triple.
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

TEST_F(PatchTreeTest, PatchIterator) {
    Patch patch1;
    patch1.add(PatchElement(Triple("g", "p", "o"), false));
    patch1.add(PatchElement(Triple("a", "p", "o"), true));
    patch1.add(PatchElement(Triple("s", "z", "o"), false));
    patch1.add(PatchElement(Triple("s", "a", "o"), true));
    patchTree->append(patch1, 1);

    Patch patch2;
    patch2.add(PatchElement(Triple("q", "p", "o"), false));
    patch2.add(PatchElement(Triple("g", "p", "o"), true));
    patch2.add(PatchElement(Triple("s", "z", "o"), false));
    patch2.add(PatchElement(Triple("s", "a", "o"), true));
    patchTree->append(patch2, 2);

    Patch patch3;
    patch3.add(PatchElement(Triple("g", "p", "o"), false));
    patch3.add(PatchElement(Triple("a", "p", "o"), false));
    patch3.add(PatchElement(Triple("h", "z", "o"), false));
    patch3.add(PatchElement(Triple("l", "a", "o"), true));
    patchTree->append(patch3, 3);

    PatchTreeIterator it = patchTree->iterator(2); // Iterate over all elements of patch 2
    PatchTreeKey key;
    PatchTreeValue value;

    ASSERT_EQ(true, it.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("g p o.", key.to_string()) << "First key is incorrect";
    ASSERT_EQ(true, value.get(2).is_addition()) << "First value is incorrect";

    ASSERT_EQ(true, it.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("q p o.", key.to_string()) << "Second key is incorrect";
    ASSERT_EQ(false, value.get(2).is_addition()) << "Second value is incorrect";

    ASSERT_EQ(true, it.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", key.to_string()) << "Third key is incorrect";
    ASSERT_EQ(true, value.get(2).is_addition()) << "Third value is incorrect";

    ASSERT_EQ(true, it.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("s z o.", key.to_string()) << "Fourth key is incorrect";
    ASSERT_EQ(false, value.get(2).is_addition()) << "Fourth value is incorrect";

    ASSERT_EQ(false, it.next(&key, &value)) << "Iterator should be finished";
}

TEST_F(PatchTreeTest, ReconstructPatchSingle) {
    Patch patch1;
    patch1.add(PatchElement(Triple("g", "p", "o"), false));
    patch1.add(PatchElement(Triple("a", "p", "o"), true));
    patch1.add(PatchElement(Triple("s", "z", "o"), false));
    patch1.add(PatchElement(Triple("s", "a", "o"), true));
    patchTree->append(patch1, 1);

    Patch patch2;
    patch2.add(PatchElement(Triple("q", "p", "o"), false));
    patch2.add(PatchElement(Triple("g", "p", "o"), true));
    patch2.add(PatchElement(Triple("s", "z", "o"), false));
    patch2.add(PatchElement(Triple("s", "a", "o"), true));
    patchTree->append(patch2, 2);

    Patch patch3;
    patch3.add(PatchElement(Triple("g", "p", "o"), false));
    patch3.add(PatchElement(Triple("a", "p", "o"), false));
    patch3.add(PatchElement(Triple("h", "z", "o"), false));
    patch3.add(PatchElement(Triple("l", "a", "o"), true));
    patchTree->append(patch3, 3);

    Patch patch2_copy = patchTree->reconstruct_patch(2);
    ASSERT_EQ(patch2.to_string(), patch2_copy.to_string()) << "Reconstructed patch should be equal to inserted patch";
}

TEST_F(PatchTreeTest, ReconstructPatchComposite) {
    Patch patch1;
    patch1.add(PatchElement(Triple("g", "p", "o"), false));
    patch1.add(PatchElement(Triple("a", "p", "o"), true));
    patchTree->append(patch1, 1);

    Patch patch2;
    patch2.add(PatchElement(Triple("s", "z", "o"), false));
    patch2.add(PatchElement(Triple("s", "a", "o"), true));
    patchTree->append(patch2, 1);

    Patch patch3;
    patch3.add(PatchElement(Triple("g", "p", "o"), false));
    patch3.add(PatchElement(Triple("a", "p", "o"), false));
    patch3.add(PatchElement(Triple("h", "z", "o"), false));
    patch3.add(PatchElement(Triple("l", "a", "o"), true));
    patchTree->append(patch3, 2);

    Patch patch2_copy = patchTree->reconstruct_patch(1);
    ASSERT_EQ("a p o. (+)\n"
              "g p o. (-)\n"
              "s a o. (+)\n"
              "s z o. (-)\n", patch2_copy.to_string()) << "Reconstructed patch should be equal to the given patch";
}
