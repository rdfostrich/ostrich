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

TEST_F(PatchTreeTest, AppendUnsafeNew) {
    Patch patch;
    patch.add(PatchElement(Triple("s1", "p1", "o1"), true));
    ASSERT_EQ(0, patchTree->append_unsafe(patch, 0)) << "Appending a patch with one elements failed";
}

TEST_F(PatchTreeTest, AppendUnsafeNotNew) {
    Patch patch1;
    patch1.add(PatchElement(Triple("s1", "p1", "o1"), true));

    Patch patch2;
    patch2.add(PatchElement(Triple("s1", "p1", "o1"), true));
    ASSERT_EQ(0, patchTree->append_unsafe(patch1, 0)) << "Appending a patch with one elements failed";
    ASSERT_EQ(-1, patchTree->append_unsafe(patch2, 0)) << "Appending a patch with one elements succeeded where it should have failed";
}

TEST_F(PatchTreeTest, AppendUnsafeContains) {
    Patch patch;
    patch.add(PatchElement(Triple("s1", "p1", "o1"), true));
    patchTree->append_unsafe(patch, 0);

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

    ASSERT_EQ(true , patchTree->contains(PatchElement(Triple("s1", "p1", "o1"), true ), 0, false)) << "Contains is incorrect";
    ASSERT_EQ(false, patchTree->contains(PatchElement(Triple("s1", "p1", "o1"), false), 0, false)) << "Contains is incorrect";
    ASSERT_EQ(true , patchTree->contains(PatchElement(Triple("s1", "p1", "o1"), false), 0, true)) << "Contains is incorrect";
    ASSERT_EQ(false, patchTree->contains(PatchElement(Triple("a", "a", "a")   , false), 0, false)) << "Contains is incorrect";
    ASSERT_EQ(false, patchTree->contains(PatchElement(Triple("a", "a", "a")   , false), 0, true)) << "Contains is incorrect";
}

TEST_F(PatchTreeTest, AppendNew) {
    Patch patch;
    patch.add(PatchElement(Triple("s1", "p1", "o1"), true));
    ASSERT_EQ(true, patchTree->append(patch, 0)) << "Appending a patch with one elements failed";
}

TEST_F(PatchTreeTest, AppendNotNew) {
    Patch patch1;
    patch1.add(PatchElement(Triple("s2", "p2", "o2"), true));

    Patch patch2;
    patch2.add(PatchElement(Triple("s1", "p1", "o1"), true));
    patch2.add(PatchElement(Triple("s2", "p2", "o2"), true));
    patch2.add(PatchElement(Triple("s3", "p3", "o3"), true));

    Patch patch3;
    patch3.add(PatchElement(Triple("s2", "p2", "o2"), false));

    ASSERT_EQ(true, patchTree->append(patch1, 0)) << "Appending a patch with one elements failed";
    ASSERT_EQ(false, patchTree->append(patch2, 0)) << "Appending a patch with 3 elements succeeded where it should have failed";

    ASSERT_EQ(false, patchTree->contains(PatchElement(Triple("s1", "p1", "o1"), true), 0, false)) << "Failing to append a patch should not have (some) of its element added.";
    ASSERT_EQ(true , patchTree->contains(PatchElement(Triple("s2", "p2", "o2"), true), 0, false)) << "Failing to append a patch should not have (some) of its element added.";
    ASSERT_EQ(false, patchTree->contains(PatchElement(Triple("s3", "p3", "o3"), true), 0, false)) << "Failing to append a patch should not have (some) of its element added.";

    ASSERT_EQ(false, patchTree->append(patch3, 0)) << "Appending a patch with one elements succeeded where it should have failed";
}

TEST_F(PatchTreeTest, IteratorOrder) {
    Patch patch;
    patch.add(PatchElement(Triple("g", "p", "o"), false));
    patch.add(PatchElement(Triple("a", "p", "o"), true));
    patch.add(PatchElement(Triple("s", "z", "o"), false));
    patch.add(PatchElement(Triple("s", "a", "o"), true));
    patchTree->append_unsafe(patch, 0);

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
    ASSERT_EQ(1, value.get(0).get_patch_position()) << "Second value is incorrect";

    ASSERT_EQ(true, it.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", key.to_string()) << "Third key is incorrect";
    ASSERT_EQ(true, value.get(0).is_addition()) << "Third value is incorrect";
    ASSERT_EQ(0, value.get(0).get_patch_id()) << "Third value is incorrect";
    ASSERT_EQ(2, value.get(0).get_patch_position()) << "Third value is incorrect";

    ASSERT_EQ(true, it.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("s z o.", key.to_string()) << "Fourth key is incorrect";
    ASSERT_EQ(false, value.get(0).is_addition()) << "Fourth value is incorrect";
    ASSERT_EQ(0, value.get(0).get_patch_id()) << "Fourth value is incorrect";
    ASSERT_EQ(3, value.get(0).get_patch_position()) << "Fourth value is incorrect";

    ASSERT_EQ(false, it.next(&key, &value)) << "Iterator should be finished";
}

TEST_F(PatchTreeTest, PatchIterator) {
    Patch patch1;
    patch1.add(PatchElement(Triple("g", "p", "o"), false));
    patch1.add(PatchElement(Triple("a", "p", "o"), true));
    patch1.add(PatchElement(Triple("s", "z", "o"), false));
    patch1.add(PatchElement(Triple("s", "a", "o"), true));
    patchTree->append_unsafe(patch1, 1);

    Patch patch2;
    patch2.add(PatchElement(Triple("q", "p", "o"), false));
    patch2.add(PatchElement(Triple("g", "p", "o"), true));
    patch2.add(PatchElement(Triple("s", "z", "o"), false));
    patch2.add(PatchElement(Triple("s", "a", "o"), true));
    patchTree->append_unsafe(patch2, 2);

    Patch patch3;
    patch3.add(PatchElement(Triple("g", "p", "o"), false));
    patch3.add(PatchElement(Triple("a", "p", "o"), false));
    patch3.add(PatchElement(Triple("h", "z", "o"), false));
    patch3.add(PatchElement(Triple("l", "a", "o"), true));
    patchTree->append_unsafe(patch3, 3);

    PatchTreeKey iteratorKey = Triple("s", "a", "o");
    PatchTreeIterator it = patchTree->iterator(&iteratorKey, 2); // Iterate over all elements of patch 2 starting from "s a o."
    PatchTreeKey key;
    PatchTreeValue value;

    ASSERT_EQ(true, it.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", key.to_string()) << "Third key is incorrect";
    ASSERT_EQ(true, value.get(2).is_addition()) << "Third value is incorrect";

    ASSERT_EQ(true, it.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("s z o.", key.to_string()) << "Fourth key is incorrect";
    ASSERT_EQ(false, value.get(2).is_addition()) << "Fourth value is incorrect";

    ASSERT_EQ(false, it.next(&key, &value)) << "Iterator should be finished";
}

TEST_F(PatchTreeTest, OffsetFilteredPatchIterator) {
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
    patchTree->append_unsafe(patch1, 1);

    Patch patch2;
    patch2.add(PatchElement(Triple("q", "p", "o"), false));
    patch2.add(PatchElement(Triple("g", "p", "o"), true));
    patch2.add(PatchElement(Triple("s", "z", "o"), false));
    patch2.add(PatchElement(Triple("s", "a", "o"), true));
    patchTree->append_unsafe(patch2, 2);

    Patch patch3;
    patch3.add(PatchElement(Triple("g", "p", "o"), false));
    patch3.add(PatchElement(Triple("a", "p", "o"), false));
    patch3.add(PatchElement(Triple("h", "z", "o"), false));
    patch3.add(PatchElement(Triple("l", "a", "o"), true));
    patchTree->append_unsafe(patch3, 3);

    Patch patch2_copy = patchTree->reconstruct_patch(2);
    ASSERT_EQ(patch2.to_string(), patch2_copy.to_string()) << "Reconstructed patch should be equal to inserted patch";
}

TEST_F(PatchTreeTest, ReconstructPatchComposite) {
    Patch patch1;
    patch1.add(PatchElement(Triple("g", "p", "o"), false));
    patch1.add(PatchElement(Triple("a", "p", "o"), true));
    patchTree->append_unsafe(patch1, 1);

    Patch patch2;
    patch2.add(PatchElement(Triple("s", "z", "o"), false));
    patch2.add(PatchElement(Triple("s", "a", "o"), true));
    patchTree->append_unsafe(patch2, 1);

    Patch patch3;
    patch3.add(PatchElement(Triple("g", "p", "o"), false));
    patch3.add(PatchElement(Triple("a", "p", "o"), false));
    patch3.add(PatchElement(Triple("h", "z", "o"), false));
    patch3.add(PatchElement(Triple("l", "a", "o"), true));
    patchTree->append_unsafe(patch3, 2);

    Patch patch2_copy = patchTree->reconstruct_patch(1);
    ASSERT_EQ("a p o. (+)\n"
              "g p o. (-)\n"
              "s a o. (+)\n"
              "s z o. (-)\n", patch2_copy.to_string()) << "Reconstructed patch should be equal to the given patch";
}

TEST_F(PatchTreeTest, RelativePatchPositions) {
    Patch patch1;
    patch1.add(PatchElement(Triple("g", "p", "o"), false));
    patch1.add(PatchElement(Triple("a", "p", "o"), true));
    patchTree->append_unsafe(patch1, 1);

    Patch patch2;
    patch2.add(PatchElement(Triple("s", "z", "o"), false));
    patch2.add(PatchElement(Triple("s", "a", "o"), true));
    patchTree->append_unsafe(patch2, 1);

    Patch patch3;
    patch3.add(PatchElement(Triple("g", "p", "o"), false));
    patch3.add(PatchElement(Triple("a", "p", "o"), false));
    patch3.add(PatchElement(Triple("h", "z", "o"), false));
    patch3.add(PatchElement(Triple("l", "a", "o"), true));
    patchTree->append_unsafe(patch3, 2);

    Patch patch4;
    patch4.add(PatchElement(Triple("h", "p", "o"), false));
    patch4.add(PatchElement(Triple("s", "z", "o"), false));
    patchTree->append_unsafe(patch4, 4);

    PatchTreeIterator it = patchTree->iterator(1); // Iterate over all elements of patch 1
    PatchTreeKey key;
    PatchTreeValue value;

    ASSERT_EQ(true, it.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("a p o.", key.to_string()) << "First key is incorrect";
    ASSERT_EQ(0, value.get(1).get_patch_position()) << "Found value is incorrect";

    ASSERT_EQ(true, it.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("g p o.", key.to_string()) << "Second key is incorrect";
    ASSERT_EQ(1, value.get(1).get_patch_position()) << "Second value is incorrect";

    ASSERT_EQ(true, it.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", key.to_string()) << "Third key is incorrect";
    ASSERT_EQ(2, value.get(1).get_patch_position()) << "Third value is incorrect";

    ASSERT_EQ(true, it.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("s z o.", key.to_string()) << "Fourth key is incorrect";
    ASSERT_EQ(3, value.get(1).get_patch_position()) << "Fourth value is incorrect";

    ASSERT_EQ(false, it.next(&key, &value)) << "Iterator should be finished";

    Patch patch5;
    patch5.add(PatchElement(Triple("a", "a", "a"), false));
    patch5.add(PatchElement(Triple("z", "z", "z"), false));
    patchTree->append_unsafe(patch5, 1);

    PatchTreeIterator it2 = patchTree->iterator(1); // Iterate over all elements of patch 1 again

    ASSERT_EQ(true, it2.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("a a a.", key.to_string()) << "First key is incorrect";
    ASSERT_EQ(0, value.get(1).get_patch_position()) << "Found value is incorrect";

    ASSERT_EQ(true, it2.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("a p o.", key.to_string()) << "Second key is incorrect";
    ASSERT_EQ(1, value.get(1).get_patch_position()) << "Second value is incorrect";

    ASSERT_EQ(true, it2.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("g p o.", key.to_string()) << "Third key is incorrect";
    ASSERT_EQ(2, value.get(1).get_patch_position()) << "Third value is incorrect";

    ASSERT_EQ(true, it2.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", key.to_string()) << "Fourth key is incorrect";
    ASSERT_EQ(3, value.get(1).get_patch_position()) << "Fourth value is incorrect";

    ASSERT_EQ(true, it2.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("s z o.", key.to_string()) << "Fifth key is incorrect";
    ASSERT_EQ(4, value.get(1).get_patch_position()) << "Fifth value is incorrect";

    ASSERT_EQ(true, it2.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("z z z.", key.to_string()) << "Sixth key is incorrect";
    ASSERT_EQ(5, value.get(1).get_patch_position()) << "Sixth value is incorrect";

    ASSERT_EQ(false, it2.next(&key, &value)) << "Iterator should be finished";
}
