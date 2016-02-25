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
    patchTree->append_unsafe(patch, 0);
}

TEST_F(PatchTreeTest, AppendUnsafeContains) {
    Patch patch;
    patch.add(PatchElement(Triple("s1", "p1", "o1"), false));
    patchTree->append_unsafe(patch, 0);

    PatchTreeKey iteratorKey = Triple("s1", "p1", "o1");
    PatchTreeIterator it = patchTree->iterator(&iteratorKey);
    PatchTreeKey key;
    PatchTreeValue value;

    ASSERT_EQ(true, it.next(&key, &value)) << "Iterator does not contain an element after append";
    ASSERT_EQ("s1 p1 o1.", key.to_string()) << "Found key is incorrect";
    ASSERT_EQ(false, value.get(0).is_addition()) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(0).get_patch_id()) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(0).get_patch_positions().sp_) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(0).get_patch_positions().s_o) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(0).get_patch_positions().s__) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(0).get_patch_positions()._po) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(0).get_patch_positions()._p_) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(0).get_patch_positions().__o) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(0).get_patch_positions().___) << "Found value is incorrect";

    ASSERT_EQ(false, it.next(&key, &value)) << "Iterator contains another element after a single append";

    ASSERT_EQ(false, patchTree->contains(PatchElement(Triple("s1", "p1", "o1"), true ), 0, false)) << "Contains is incorrect";
    ASSERT_EQ(true , patchTree->contains(PatchElement(Triple("s1", "p1", "o1"), false), 0, false)) << "Contains is incorrect";
    ASSERT_EQ(true , patchTree->contains(PatchElement(Triple("s1", "p1", "o1"), true), 0, true)) << "Contains is incorrect";
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
    // Expected order:
    // a p o +
    // g p o -
    // s a o +
    // s z o -
    patchTree->append_unsafe(patch, 0);

    PatchTreeKey iteratorKey = Triple("a", "a", "a");
    PatchTreeIterator it = patchTree->iterator(&iteratorKey); // Iterate starting from the given triple.
    PatchTreeKey key;
    PatchTreeValue value;

    ASSERT_EQ(true, it.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("a p o.", key.to_string()) << "First key is incorrect";
    ASSERT_EQ(true, value.get(0).is_addition()) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(0).get_patch_id()) << "Found value is incorrect";
    ASSERT_EQ(-1, value.get(0).get_patch_positions().sp_) << "Found value is incorrect";
    ASSERT_EQ(-1, value.get(0).get_patch_positions().s_o) << "Found value is incorrect";
    ASSERT_EQ(-1, value.get(0).get_patch_positions().s__) << "Found value is incorrect";
    ASSERT_EQ(-1, value.get(0).get_patch_positions()._po) << "Found value is incorrect";
    ASSERT_EQ(-1, value.get(0).get_patch_positions()._p_) << "Found value is incorrect";
    ASSERT_EQ(-1, value.get(0).get_patch_positions().__o) << "Found value is incorrect";
    ASSERT_EQ(-1, value.get(0).get_patch_positions().___) << "Found value is incorrect";

    ASSERT_EQ(true, it.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("g p o.", key.to_string()) << "Second key is incorrect";
    ASSERT_EQ(false, value.get(0).is_addition()) << "Second value is incorrect";
    ASSERT_EQ(0, value.get(0).get_patch_id()) << "Second value is incorrect";
    ASSERT_EQ(0, value.get(0).get_patch_positions().sp_) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(0).get_patch_positions().s_o) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(0).get_patch_positions().s__) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(0).get_patch_positions()._po) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(0).get_patch_positions()._p_) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(0).get_patch_positions().__o) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(0).get_patch_positions().___) << "Found value is incorrect";

    ASSERT_EQ(true, it.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", key.to_string()) << "Third key is incorrect";
    ASSERT_EQ(true, value.get(0).is_addition()) << "Third value is incorrect";
    ASSERT_EQ(0, value.get(0).get_patch_id()) << "Third value is incorrect";
    ASSERT_EQ(-1, value.get(0).get_patch_positions().sp_) << "Found value is incorrect";
    ASSERT_EQ(-1, value.get(0).get_patch_positions().s_o) << "Found value is incorrect";
    ASSERT_EQ(-1, value.get(0).get_patch_positions().s__) << "Found value is incorrect";
    ASSERT_EQ(-1, value.get(0).get_patch_positions()._po) << "Found value is incorrect";
    ASSERT_EQ(-1, value.get(0).get_patch_positions()._p_) << "Found value is incorrect";
    ASSERT_EQ(-1, value.get(0).get_patch_positions().__o) << "Found value is incorrect";
    ASSERT_EQ(-1, value.get(0).get_patch_positions().___) << "Found value is incorrect";

    ASSERT_EQ(true, it.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("s z o.", key.to_string()) << "Fourth key is incorrect";
    ASSERT_EQ(false, value.get(0).is_addition()) << "Fourth value is incorrect";
    ASSERT_EQ(0, value.get(0).get_patch_id()) << "Fourth value is incorrect";
    ASSERT_EQ(0, value.get(0).get_patch_positions().sp_) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(0).get_patch_positions().s_o) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(0).get_patch_positions().s__) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(0).get_patch_positions()._po) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(0).get_patch_positions()._p_) << "Found value is incorrect";
    ASSERT_EQ(1, value.get(0).get_patch_positions().__o) << "Found value is incorrect";
    ASSERT_EQ(1, value.get(0).get_patch_positions().___) << "Found value is incorrect";

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
    PatchTreeIterator it = patchTree->iterator(&iteratorKey, 2, false); // Iterate over all elements of patch 2 starting from "s a o."
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

    PatchTreeIterator it = patchTree->iterator(2, false); // Iterate over all elements of patch only 2
    PatchTreeKey key;
    PatchTreeValue value;

    ASSERT_EQ(true, it.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("a p o.", key.to_string()) << "First key is incorrect";
    ASSERT_EQ(true, value.get(2).is_addition()) << "First value is incorrect";

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

    PatchTreeIterator it2 = patchTree->iterator(2, false); // Iterate over all elements of patch 2 and before

    ASSERT_EQ(true, it2.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("a p o.", key.to_string()) << "First key is incorrect";
    ASSERT_EQ(true, value.get(2).is_addition()) << "First value is incorrect";

    ASSERT_EQ(true, it2.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("g p o.", key.to_string()) << "First key is incorrect";
    ASSERT_EQ(true, value.get(2).is_addition()) << "First value is incorrect";

    ASSERT_EQ(true, it2.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("q p o.", key.to_string()) << "Second key is incorrect";
    ASSERT_EQ(false, value.get(2).is_addition()) << "Second value is incorrect";

    ASSERT_EQ(true, it2.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", key.to_string()) << "Third key is incorrect";
    ASSERT_EQ(true, value.get(2).is_addition()) << "Third value is incorrect";

    ASSERT_EQ(true, it2.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("s z o.", key.to_string()) << "Fourth key is incorrect";
    ASSERT_EQ(false, value.get(2).is_addition()) << "Fourth value is incorrect";

    ASSERT_EQ(false, it2.next(&key, &value)) << "Iterator should be finished";
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
    ASSERT_EQ("a p o. (+)\ng p o. (+)\nq p o. (-)\ns a o. (+)\ns z o. (-)\n", patch2_copy.to_string()) << "Reconstructed patch should be equal to inserted patch";
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

TEST_F(PatchTreeTest, ReconstructPatchComposite2) {
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

    Patch patch4;
    patch4.add(PatchElement(Triple("h", "p", "o"), false));
    patch4.add(PatchElement(Triple("s", "z", "o"), false));
    patchTree->append(patch4, 4);

    Patch patch5;
    patch5.add(PatchElement(Triple("h", "p", "o"), true));
    patchTree->append(patch5, 5);

    Patch patch1_copy = patchTree->reconstruct_patch(1);
    ASSERT_EQ("a p o. (+)\n"
              "g p o. (-)\n"
              "s a o. (+)\n"
              "s z o. (-)\n", patch1_copy.to_string()) << "Reconstructed patch should be equal to the given patch";

    Patch patch2_copy = patchTree->reconstruct_patch(2);
    ASSERT_EQ("a p o. (-)\n"
              "g p o. (-)\n"
              "h z o. (-)\n"
              "l a o. (+)\n"
              "s a o. (+)\n"
              "s z o. (-)\n", patch2_copy.to_string()) << "Reconstructed patch should be equal to the given patch";

    Patch patch4_copy = patchTree->reconstruct_patch(4);
    ASSERT_EQ("a p o. (-)\n"
              "g p o. (-)\n"
              "h p o. (-)\n"
              "h z o. (-)\n"
              "l a o. (+)\n"
              "s a o. (+)\n"
              "s z o. (-)\n", patch4_copy.to_string()) << "Reconstructed patch should be equal to the given patch";

    Patch patch5_copy = patchTree->reconstruct_patch(5);
    ASSERT_EQ("a p o. (-)\n"
              "g p o. (-)\n"
              "h p o. (+)\n"
              "h z o. (-)\n"
              "l a o. (+)\n"
              "s a o. (+)\n"
              "s z o. (-)\n", patch5_copy.to_string()) << "Reconstructed patch should be equal to the given patch";
}

TEST_F(PatchTreeTest, RelativePatchPositions) {
    Patch patch1;
    patch1.add(PatchElement(Triple("g", "p", "o"), false));
    patch1.add(PatchElement(Triple("a", "p", "o"), false));
    patchTree->append_unsafe(patch1, 1);

    Patch patch2;
    patch2.add(PatchElement(Triple("s", "z", "o"), false));
    patch2.add(PatchElement(Triple("a", "b", "c"), true));
    patch2.add(PatchElement(Triple("s", "a", "o"), false));
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

    PatchTreeIterator it = patchTree->iterator(1, false); // Iterate over all elements of patch 1
    PatchTreeKey key;
    PatchTreeValue value;

    // Expected order for patch 1:
    // a b c +
    // a p o -
    // g p o -
    // s a o -
    // s z o -

    ASSERT_EQ(true, it.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("a b c.", key.to_string()) << "First key is incorrect";
    ASSERT_EQ(-1, value.get(1).get_patch_positions().sp_) << "Found value is incorrect";
    ASSERT_EQ(-1, value.get(1).get_patch_positions().s_o) << "Found value is incorrect";
    ASSERT_EQ(-1, value.get(1).get_patch_positions().s__) << "Found value is incorrect";
    ASSERT_EQ(-1, value.get(1).get_patch_positions()._po) << "Found value is incorrect";
    ASSERT_EQ(-1, value.get(1).get_patch_positions()._p_) << "Found value is incorrect";
    ASSERT_EQ(-1, value.get(1).get_patch_positions().__o) << "Found value is incorrect";
    ASSERT_EQ(-1, value.get(1).get_patch_positions().___) << "Found value is incorrect";

    ASSERT_EQ(true, it.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("a p o.", key.to_string()) << "First key is incorrect";
    ASSERT_EQ(0, value.get(1).get_patch_positions().sp_) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(1).get_patch_positions().s_o) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(1).get_patch_positions().s__) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(1).get_patch_positions()._po) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(1).get_patch_positions()._p_) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(1).get_patch_positions().__o) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(1).get_patch_positions().___) << "Found value is incorrect";

    ASSERT_EQ(true, it.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("g p o.", key.to_string()) << "Second key is incorrect";
    ASSERT_EQ(0, value.get(1).get_patch_positions().sp_) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(1).get_patch_positions().s_o) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(1).get_patch_positions().s__) << "Found value is incorrect";
    ASSERT_EQ(1, value.get(1).get_patch_positions()._po) << "Found value is incorrect";
    ASSERT_EQ(1, value.get(1).get_patch_positions()._p_) << "Found value is incorrect";
    ASSERT_EQ(1, value.get(1).get_patch_positions().__o) << "Found value is incorrect";
    ASSERT_EQ(1, value.get(1).get_patch_positions().___) << "Found value is incorrect";

    ASSERT_EQ(true, it.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", key.to_string()) << "Third key is incorrect";
    ASSERT_EQ(0, value.get(1).get_patch_positions().sp_) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(1).get_patch_positions().s_o) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(1).get_patch_positions().s__) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(1).get_patch_positions()._po) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(1).get_patch_positions()._p_) << "Found value is incorrect";
    ASSERT_EQ(2, value.get(1).get_patch_positions().__o) << "Found value is incorrect";
    ASSERT_EQ(2, value.get(1).get_patch_positions().___) << "Found value is incorrect";

    ASSERT_EQ(true, it.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("s z o.", key.to_string()) << "Fourth key is incorrect";
    ASSERT_EQ(0, value.get(1).get_patch_positions().sp_) << "Found value is incorrect";
    ASSERT_EQ(1, value.get(1).get_patch_positions().s_o) << "Found value is incorrect";
    ASSERT_EQ(1, value.get(1).get_patch_positions().s__) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(1).get_patch_positions()._po) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(1).get_patch_positions()._p_) << "Found value is incorrect";
    ASSERT_EQ(3, value.get(1).get_patch_positions().__o) << "Found value is incorrect";
    ASSERT_EQ(3, value.get(1).get_patch_positions().___) << "Found value is incorrect";

    ASSERT_EQ(false, it.next(&key, &value)) << "Iterator should be finished";

    Patch patch5;
    patch5.add(PatchElement(Triple("a", "a", "a"), false));
    patch5.add(PatchElement(Triple("z", "z", "z"), false));
    patchTree->append_unsafe(patch5, 1);

    PatchTreeIterator it2 = patchTree->iterator(1, false); // Iterate over all elements of patch 1 again

    // Expected order for patch 1:
    // a a a -
    // a b c +
    // a p o -
    // g p o -
    // s a o -
    // s z o -
    // z z z -

    ASSERT_EQ(true, it2.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("a a a.", key.to_string()) << "First key is incorrect";
    ASSERT_EQ(0, value.get(1).get_patch_positions().sp_) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(1).get_patch_positions().s_o) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(1).get_patch_positions().s__) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(1).get_patch_positions()._po) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(1).get_patch_positions()._p_) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(1).get_patch_positions().__o) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(1).get_patch_positions().___) << "Found value is incorrect";

    ASSERT_EQ(true, it2.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("a b c.", key.to_string()) << "First key is incorrect";
    ASSERT_EQ(-1, value.get(1).get_patch_positions().sp_) << "Found value is incorrect";
    ASSERT_EQ(-1, value.get(1).get_patch_positions().s_o) << "Found value is incorrect";
    ASSERT_EQ(-1, value.get(1).get_patch_positions().s__) << "Found value is incorrect";
    ASSERT_EQ(-1, value.get(1).get_patch_positions()._po) << "Found value is incorrect";
    ASSERT_EQ(-1, value.get(1).get_patch_positions()._p_) << "Found value is incorrect";
    ASSERT_EQ(-1, value.get(1).get_patch_positions().__o) << "Found value is incorrect";
    ASSERT_EQ(-1, value.get(1).get_patch_positions().___) << "Found value is incorrect";

    ASSERT_EQ(true, it2.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("a p o.", key.to_string()) << "Second key is incorrect";
    ASSERT_EQ(0, value.get(1).get_patch_positions().sp_) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(1).get_patch_positions().s_o) << "Found value is incorrect";
    ASSERT_EQ(1, value.get(1).get_patch_positions().s__) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(1).get_patch_positions()._po) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(1).get_patch_positions()._p_) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(1).get_patch_positions().__o) << "Found value is incorrect";
    ASSERT_EQ(1, value.get(1).get_patch_positions().___) << "Found value is incorrect";

    ASSERT_EQ(true, it2.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("g p o.", key.to_string()) << "Third key is incorrect";
    ASSERT_EQ(0, value.get(1).get_patch_positions().sp_) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(1).get_patch_positions().s_o) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(1).get_patch_positions().s__) << "Found value is incorrect";
    ASSERT_EQ(1, value.get(1).get_patch_positions()._po) << "Found value is incorrect";
    ASSERT_EQ(1, value.get(1).get_patch_positions()._p_) << "Found value is incorrect";
    ASSERT_EQ(1, value.get(1).get_patch_positions().__o) << "Found value is incorrect";
    ASSERT_EQ(2, value.get(1).get_patch_positions().___) << "Found value is incorrect";

    ASSERT_EQ(true, it2.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", key.to_string()) << "Fourth key is incorrect";
    ASSERT_EQ(0, value.get(1).get_patch_positions().sp_) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(1).get_patch_positions().s_o) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(1).get_patch_positions().s__) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(1).get_patch_positions()._po) << "Found value is incorrect";
    ASSERT_EQ(1, value.get(1).get_patch_positions()._p_) << "Found value is incorrect";
    ASSERT_EQ(2, value.get(1).get_patch_positions().__o) << "Found value is incorrect";
    ASSERT_EQ(3, value.get(1).get_patch_positions().___) << "Found value is incorrect";

    ASSERT_EQ(true, it2.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("s z o.", key.to_string()) << "Fifth key is incorrect";
    ASSERT_EQ(0, value.get(1).get_patch_positions().sp_) << "Found value is incorrect";
    ASSERT_EQ(1, value.get(1).get_patch_positions().s_o) << "Found value is incorrect";
    ASSERT_EQ(1, value.get(1).get_patch_positions().s__) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(1).get_patch_positions()._po) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(1).get_patch_positions()._p_) << "Found value is incorrect";
    ASSERT_EQ(3, value.get(1).get_patch_positions().__o) << "Found value is incorrect";
    ASSERT_EQ(4, value.get(1).get_patch_positions().___) << "Found value is incorrect";

    ASSERT_EQ(true, it2.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("z z z.", key.to_string()) << "Sixth key is incorrect";
    ASSERT_EQ(0, value.get(1).get_patch_positions().sp_) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(1).get_patch_positions().s_o) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(1).get_patch_positions().s__) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(1).get_patch_positions()._po) << "Found value is incorrect";
    ASSERT_EQ(1, value.get(1).get_patch_positions()._p_) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(1).get_patch_positions().__o) << "Found value is incorrect";
    ASSERT_EQ(5, value.get(1).get_patch_positions().___) << "Found value is incorrect";

    ASSERT_EQ(false, it2.next(&key, &value)) << "Iterator should be finished";
}

TEST_F(PatchTreeTest, RelativePatchPositions2) {
    Patch patch1;
    patch1.add(PatchElement(Triple("a", "b", "c"), true));
    patch1.add(PatchElement(Triple("a", "a", "a"), false));
    patch1.add(PatchElement(Triple("z", "z", "z"), false));
    patchTree->append_unsafe(patch1, 1);

    Patch patch2;
    patch2.add(PatchElement(Triple("a", "b", "c"), false));
    patchTree->append_unsafe(patch2, 2);

    PatchTreeIterator it1 = patchTree->iterator(1, false); // Iterate over all elements of patch 1
    PatchTreeKey key;
    PatchTreeValue value;

    // Expected order for patch 1:
    // a a a -
    // a b c +
    // z z z -

    ASSERT_EQ(true, it1.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("a a a.", key.to_string()) << "Key is incorrect";
    ASSERT_EQ(0, value.get(1).get_patch_positions().sp_) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(1).get_patch_positions().s_o) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(1).get_patch_positions().s__) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(1).get_patch_positions()._po) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(1).get_patch_positions()._p_) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(1).get_patch_positions().__o) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(1).get_patch_positions().___) << "Found value is incorrect";

    ASSERT_EQ(true, it1.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("a b c.", key.to_string()) << "Key is incorrect";
    ASSERT_EQ(-1, value.get(1).get_patch_positions().sp_) << "Found value is incorrect";
    ASSERT_EQ(-1, value.get(1).get_patch_positions().s_o) << "Found value is incorrect";
    ASSERT_EQ(-1, value.get(1).get_patch_positions().s__) << "Found value is incorrect";
    ASSERT_EQ(-1, value.get(1).get_patch_positions()._po) << "Found value is incorrect";
    ASSERT_EQ(-1, value.get(1).get_patch_positions()._p_) << "Found value is incorrect";
    ASSERT_EQ(-1, value.get(1).get_patch_positions().__o) << "Found value is incorrect";
    ASSERT_EQ(-1, value.get(1).get_patch_positions().___) << "Found value is incorrect";

    ASSERT_EQ(true, it1.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("z z z.", key.to_string()) << "Key is incorrect";
    ASSERT_EQ(0, value.get(1).get_patch_positions().sp_) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(1).get_patch_positions().s_o) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(1).get_patch_positions().s__) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(1).get_patch_positions()._po) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(1).get_patch_positions()._p_) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(1).get_patch_positions().__o) << "Found value is incorrect";
    ASSERT_EQ(1, value.get(1).get_patch_positions().___) << "Found value is incorrect";

    ASSERT_EQ(false, it1.next(&key, &value)) << "Iterator should be finished";

    PatchTreeIterator it2 = patchTree->iterator(2, false); // Iterate over all elements of patch 2
    // Expected order for patch 2:
    // a a a -
    // a b c -
    // z z z -

    ASSERT_EQ(true, it2.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("a a a.", key.to_string()) << "Key is incorrect";
    ASSERT_EQ(0, value.get(2).get_patch_positions().sp_) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(2).get_patch_positions().s_o) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(2).get_patch_positions().s__) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(2).get_patch_positions()._po) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(2).get_patch_positions()._p_) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(2).get_patch_positions().__o) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(2).get_patch_positions().___) << "Found value is incorrect";

    ASSERT_EQ(true, it2.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("a b c.", key.to_string()) << "Key is incorrect";
    // The positions are -1, this means that even though this is a deletion, it
    // is not a deletion relative to the snapshot, but only internally inside the patch. (it did not exist in the snapshot)
    ASSERT_EQ(-1, value.get(2).get_patch_positions().sp_) << "Found value is incorrect";
    ASSERT_EQ(-1, value.get(2).get_patch_positions().s_o) << "Found value is incorrect";
    ASSERT_EQ(-1, value.get(2).get_patch_positions().s__) << "Found value is incorrect";
    ASSERT_EQ(-1, value.get(2).get_patch_positions()._po) << "Found value is incorrect";
    ASSERT_EQ(-1, value.get(2).get_patch_positions()._p_) << "Found value is incorrect";
    ASSERT_EQ(-1, value.get(2).get_patch_positions().__o) << "Found value is incorrect";
    ASSERT_EQ(-1, value.get(2).get_patch_positions().___) << "Found value is incorrect";

    ASSERT_EQ(true, it2.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("z z z.", key.to_string()) << "Key is incorrect";
    ASSERT_EQ(0, value.get(2).get_patch_positions().sp_) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(2).get_patch_positions().s_o) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(2).get_patch_positions().s__) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(2).get_patch_positions()._po) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(2).get_patch_positions()._p_) << "Found value is incorrect";
    ASSERT_EQ(0, value.get(2).get_patch_positions().__o) << "Found value is incorrect";
    ASSERT_EQ(1, value.get(2).get_patch_positions().___) << "Found value is incorrect";

    ASSERT_EQ(false, it2.next(&key, &value)) << "Iterator should be finished";
}

TEST_F(PatchTreeTest, DeletionCount) {
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

    Patch patch4;
    patch4.add(PatchElement(Triple("h", "p", "o"), false));
    patch4.add(PatchElement(Triple("s", "z", "o"), false));
    patchTree->append(patch4, 4);

    // Patch 1
    // a p o +
    // g p o -
    // s a o +
    // s z o -

    ASSERT_EQ(2, patchTree->deletion_count(Triple("", "", ""), 1)) << "Deletion count is incorrect";
    ASSERT_EQ(1, patchTree->deletion_count(Triple("s", "", ""), 1)) << "Deletion count is incorrect";
    ASSERT_EQ(0, patchTree->deletion_count(Triple("s", "a", ""), 1)) << "Deletion count is incorrect";
    ASSERT_EQ(2, patchTree->deletion_count(Triple("", "", "o"), 1)) << "Deletion count is incorrect";
    ASSERT_EQ(1, patchTree->deletion_count(Triple("", "p", "o"), 1)) << "Deletion count is incorrect";
    ASSERT_EQ(1, patchTree->deletion_count(Triple("", "p", ""), 1)) << "Deletion count is incorrect";
    ASSERT_EQ(1, patchTree->deletion_count(Triple("g", "p", ""), 1)) << "Deletion count is incorrect";

    // Patch 2
    // a p o +/-
    // g p o -/- (=> ignored in count)
    // h z o -
    // l a o +
    // s a o +
    // s z o -

    ASSERT_EQ(3, patchTree->deletion_count(Triple("", "", ""), 2)) << "Deletion count is incorrect";

    // Patch 4
    // a p o +/-
    // g p o -/- (=> ignored in count)
    // h p o -
    // h z o -
    // l a o +
    // s a o +
    // s z o -/-

    ASSERT_EQ(4, patchTree->deletion_count(Triple("", "", ""), 4)) << "Deletion count is incorrect";
}

TEST_F(PatchTreeTest, DeletionIterator) {
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

    Patch patch4;
    patch4.add(PatchElement(Triple("h", "p", "o"), false));
    patch4.add(PatchElement(Triple("s", "z", "o"), false));
    patchTree->append(patch4, 4);

    // Expected patch 1:
    // a p o +
    // g p o -
    // s a o +
    // s z o -

    // Expected patch 2:
    // a p o +/- (=> ignored)
    // g p o -
    // h z o -
    // l a o +
    // s a o +
    // s z o -

    // Expected patch 4:
    // a p o +/- (=> ignored)
    // g p o -
    // h p o -
    // h z o -
    // l a o +
    // s a o +
    // s z o -

    PositionedTriple pt;

    /*
     * Looping over all deletions in patch 1 starting from beginning
     */
    PositionedTripleIterator it1 = patchTree->deletion_iterator_from(Triple("", "", ""), 1, Triple("", "", ""));

    ASSERT_EQ(true, it1.next(&pt)) << "Iterator has a no next value";
    ASSERT_EQ("g p o.", pt.triple.to_string()) << "Element is incorrect";
    ASSERT_EQ(0, pt.position) << "Element is incorrect";

    ASSERT_EQ(true, it1.next(&pt)) << "Iterator has a no next value";
    ASSERT_EQ("s z o.", pt.triple.to_string()) << "Element is incorrect";
    ASSERT_EQ(1, pt.position) << "Element is incorrect";

    ASSERT_EQ(false, it1.next(&pt)) << "Iterator should be finished";

    /*
     * Looping over all deletions in patch 1 starting from s z o
     */
    PositionedTripleIterator it2 = patchTree->deletion_iterator_from(Triple("s", "z", "o"), 1, Triple("", "", ""));

    ASSERT_EQ(true, it2.next(&pt)) << "Iterator has a no next value";
    ASSERT_EQ("s z o.", pt.triple.to_string()) << "Element is incorrect";
    ASSERT_EQ(1, pt.position) << "Element is incorrect";

    /*
     * Looping over ? z ? deletions in patch 1 starting from s z o
     */
    PositionedTripleIterator it3 = patchTree->deletion_iterator_from(Triple("s", "z", "o"), 1, Triple("", "z", ""));

    ASSERT_EQ(true, it3.next(&pt)) << "Iterator has a no next value";
    ASSERT_EQ("s z o.", pt.triple.to_string()) << "Element is incorrect";
    ASSERT_EQ(0, pt.position) << "Element is incorrect";

    ASSERT_EQ(false, it3.next(&pt)) << "Iterator should be finished";

    /*
     * Looping over ? ? o deletions in patch 1 starting from a non-existing start element that lies before all other elements
     */
    PositionedTripleIterator it4 = patchTree->deletion_iterator_from(Triple("b", "b", "b"), 1, Triple("", "", "o"));

    ASSERT_EQ(true, it4.next(&pt)) << "Iterator has a no next value";
    ASSERT_EQ("g p o.", pt.triple.to_string()) << "Element is incorrect";
    ASSERT_EQ(0, pt.position) << "Element is incorrect";
    
    ASSERT_EQ(true, it4.next(&pt)) << "Iterator has a no next value";
    ASSERT_EQ("s z o.", pt.triple.to_string()) << "Element is incorrect";
    ASSERT_EQ(1, pt.position) << "Element is incorrect";

    ASSERT_EQ(false, it4.next(&pt)) << "Iterator should be finished";

    /*
     * Looping over ? ? o deletions in patch 1 starting from a non-existing start element that lies between the two elements
     */
    PositionedTripleIterator it5 = patchTree->deletion_iterator_from(Triple("h", "h", "h"), 1, Triple("", "", "o"));

    ASSERT_EQ(true, it5.next(&pt)) << "Iterator has a no next value";
    ASSERT_EQ("s z o.", pt.triple.to_string()) << "Element is incorrect";
    ASSERT_EQ(1, pt.position) << "Element is incorrect";

    ASSERT_EQ(false, it5.next(&pt)) << "Iterator should be finished";

    /*
     * Looping over ? ? o deletions in patch 1 starting from a non-existing start element that lies after the two elements
     */
    PositionedTripleIterator it6 = patchTree->deletion_iterator_from(Triple("t", "t", "t"), 1, Triple("", "", "o"));
    ASSERT_EQ(false, it6.next(&pt)) << "Iterator should be finished";

    /*
     * Looping over all deletions in patch 4 starting from beginning
     */
    PositionedTripleIterator it7 = patchTree->deletion_iterator_from(Triple("", "", ""), 4, Triple("", "", ""));

    ASSERT_EQ(true, it7.next(&pt)) << "Iterator has a no next value";
    ASSERT_EQ("g p o.", pt.triple.to_string()) << "Element is incorrect";
    ASSERT_EQ(0, pt.position) << "Element is incorrect";

    ASSERT_EQ(true, it7.next(&pt)) << "Iterator has a no next value";
    ASSERT_EQ("h p o.", pt.triple.to_string()) << "Element is incorrect";
    ASSERT_EQ(1, pt.position) << "Element is incorrect";

    ASSERT_EQ(true, it7.next(&pt)) << "Iterator has a no next value";
    ASSERT_EQ("h z o.", pt.triple.to_string()) << "Element is incorrect";
    ASSERT_EQ(2, pt.position) << "Element is incorrect";

    ASSERT_EQ(true, it7.next(&pt)) << "Iterator has a no next value";
    ASSERT_EQ("s z o.", pt.triple.to_string()) << "Element is incorrect";
    ASSERT_EQ(3, pt.position) << "Element is incorrect";

    ASSERT_EQ(false, it7.next(&pt)) << "Iterator should be finished";

    /*
     * Looping over h ? o deletions in patch 4 starting from g p o
     */
    PositionedTripleIterator it8 = patchTree->deletion_iterator_from(Triple("g", "p", "o"), 4, Triple("h", "", "o"));

    ASSERT_EQ(true, it8.next(&pt)) << "Iterator has a no next value";
    ASSERT_EQ("h p o.", pt.triple.to_string()) << "Element is incorrect";
    ASSERT_EQ(0, pt.position) << "Element is incorrect";

    ASSERT_EQ(true, it8.next(&pt)) << "Iterator has a no next value";
    ASSERT_EQ("h z o.", pt.triple.to_string()) << "Element is incorrect";
    ASSERT_EQ(1, pt.position) << "Element is incorrect";

    ASSERT_EQ(false, it8.next(&pt)) << "Iterator should be finished";

    /*
     * Looping over h ? o deletions in patch 4 starting from h q o
     */
    PositionedTripleIterator it9 = patchTree->deletion_iterator_from(Triple("h", "q", "o"), 4, Triple("h", "", "o"));

    ASSERT_EQ(true, it9.next(&pt)) << "Iterator has a no next value";
    ASSERT_EQ("h z o.", pt.triple.to_string()) << "Element is incorrect";
    ASSERT_EQ(1, pt.position) << "Element is incorrect";

    ASSERT_EQ(false, it9.next(&pt)) << "Iterator should be finished";
}

TEST_F(PatchTreeTest, AdditionIterator) {
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

    Patch patch4;
    patch4.add(PatchElement(Triple("h", "p", "o"), false));
    patch4.add(PatchElement(Triple("s", "z", "o"), false));
    patchTree->append(patch4, 4);

    Patch patch5;
    patch5.add(PatchElement(Triple("h", "p", "o"), true));
    patchTree->append(patch5, 5);

    // Expected patch 1:
    // a p o +
    // g p o -
    // s a o +
    // s z o -

    // Expected patch 2:
    // a p o +/- (=> ignored)
    // g p o -
    // h z o -
    // l a o +
    // s a o +
    // s z o -

    // Expected patch 4:
    // a p o +/- (=> ignored)
    // g p o -
    // h p o -
    // h z o -
    // l a o +
    // s a o +
    // s z o -/-

    // Expected patch 5:
    // a p o +/- (=> ignored)
    // g p o -
    // h p o -/+ (=> ignored)
    // h z o -
    // l a o +
    // s a o +
    // s z o -

    PositionedTriple pt;

    /*
     * Looping over all additions in patch 1 starting from beginning
     */
    PositionedTripleIterator it1 = patchTree->addition_iterator_from(0, 1, Triple("", "", ""));

    ASSERT_EQ(true, it1.next(&pt)) << "Iterator has a no next value";
    ASSERT_EQ("a p o.", pt.triple.to_string()) << "Element is incorrect";

    ASSERT_EQ(true, it1.next(&pt)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", pt.triple.to_string()) << "Element is incorrect";

    ASSERT_EQ(false, it1.next(&pt)) << "Iterator should be finished";

    /*
     * Looping over s a o additions in patch 1 starting from beginning
     */
    PositionedTripleIterator it2 = patchTree->addition_iterator_from(0, 1, Triple("s", "a", "o"));

    ASSERT_EQ(true, it2.next(&pt)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", pt.triple.to_string()) << "Element is incorrect";

    ASSERT_EQ(false, it2.next(&pt)) << "Iterator should be finished";

    /*
     * Looping over all additions in patch 1 starting from 1
     */
    PositionedTripleIterator it3 = patchTree->addition_iterator_from(1, 1, Triple("", "", ""));

    ASSERT_EQ(true, it3.next(&pt)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", pt.triple.to_string()) << "Element is incorrect";

    ASSERT_EQ(false, it3.next(&pt)) << "Iterator should be finished";

    /*
     * Looping over all additions in patch 2 starting from beginning
     */
    PositionedTripleIterator it4 = patchTree->addition_iterator_from(0, 2, Triple("", "", ""));

    ASSERT_EQ(true, it4.next(&pt)) << "Iterator has a no next value";
    ASSERT_EQ("l a o.", pt.triple.to_string()) << "Element is incorrect";

    ASSERT_EQ(true, it4.next(&pt)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", pt.triple.to_string()) << "Element is incorrect";

    ASSERT_EQ(false, it4.next(&pt)) << "Iterator should be finished";

    /*
     * Looping over all additions in patch 5 starting from beginning
     */
    PositionedTripleIterator it5 = patchTree->addition_iterator_from(0, 5, Triple("", "", ""));

    ASSERT_EQ(true, it5.next(&pt)) << "Iterator has a no next value";
    ASSERT_EQ("l a o.", pt.triple.to_string()) << "Element is incorrect";

    ASSERT_EQ(true, it5.next(&pt)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", pt.triple.to_string()) << "Element is incorrect";

    ASSERT_EQ(false, it5.next(&pt)) << "Iterator should be finished";
}
