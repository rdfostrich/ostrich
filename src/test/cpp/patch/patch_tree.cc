#include <gtest/gtest.h>

#include "../../../main/cpp/patch/patch_tree.h"
#include "../../../main/cpp/dictionary/dictionary_manager.h"
#include "../../../main/cpp/patch/patch_tree_manager.h"

// The fixture for testing class PatchTree.
class PatchTreeTest : public ::testing::Test {
protected:
    PatchTree* patchTree;
    DictionaryManager dict;

    PatchTreeTest() : patchTree(NULL), dict(DictionaryManager(0)) {}

    virtual ~PatchTreeTest() {

    }

    virtual void SetUp() {
        patchTree = new PatchTree(0, &dict);
    }

    virtual void TearDown() {
        delete patchTree;
        std::remove(PATCHTREE_FILENAME(0, "spo_deletions").c_str());
        std::remove(PATCHTREE_FILENAME(0, "spo").c_str());
        std::remove(PATCHTREE_FILENAME(0, "pos").c_str());
        std::remove(PATCHTREE_FILENAME(0, "pso").c_str());
        std::remove(PATCHTREE_FILENAME(0, "sop").c_str());
        std::remove(PATCHTREE_FILENAME(0, "osp").c_str());
        std::remove(METADATA_FILENAME_BASE(0).c_str());

        DictionaryManager::cleanup(0);
    }
};

TEST_F(PatchTreeTest, AppendUnsafeNew) {
    Patch patch(&dict);
    patch.add(PatchElement(Triple("s1", "p1", "o1", &dict), true));
    patchTree->append_unsafe(patch, 0);
}

TEST_F(PatchTreeTest, AppendUnsafeContains) {
    Patch patch(&dict);
    patch.add(PatchElement(Triple("s1", "p1", "o1", &dict), false));
    patchTree->append_unsafe(patch, 0);

    PatchTreeKey iteratorKey = Triple("s1", "p1", "o1", &dict);
    PatchTreeIterator it = patchTree->iterator(&iteratorKey);
    PatchTreeKey key;
    PatchTreeValue value;

    ASSERT_EQ(true, it.next(&key, &value)) << "Iterator does not contain an element after append";
    ASSERT_EQ("s1 p1 o1.", key.to_string(dict)) << "Found key is incorrect";
    ASSERT_EQ(true, value.is_deletion(0, true)) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(0).get_patch_id()) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(0).get_patch_positions().sp_) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(0).get_patch_positions().s_o) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(0).get_patch_positions().s__) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(0).get_patch_positions()._po) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(0).get_patch_positions()._p_) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(0).get_patch_positions().__o) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(0).get_patch_positions().___) << "Found value is incorrect";

    ASSERT_EQ(false, it.next(&key, &value)) << "Iterator contains another element after a single append";

    ASSERT_EQ(false, patchTree->contains(PatchElement(Triple("s1", "p1", "o1", &dict), true ), 0, false)) << "Contains is incorrect";
    ASSERT_EQ(true , patchTree->contains(PatchElement(Triple("s1", "p1", "o1", &dict), false), 0, false)) << "Contains is incorrect";
    ASSERT_EQ(true , patchTree->contains(PatchElement(Triple("s1", "p1", "o1", &dict), true), 0, true)) << "Contains is incorrect";
    ASSERT_EQ(false, patchTree->contains(PatchElement(Triple("a", "a", "a", &dict)   , false), 0, false)) << "Contains is incorrect";
    ASSERT_EQ(false, patchTree->contains(PatchElement(Triple("a", "a", "a", &dict)   , false), 0, true)) << "Contains is incorrect";
}

TEST_F(PatchTreeTest, AppendNew) {
    Patch patch(&dict);
    patch.add(PatchElement(Triple("s1", "p1", "o1", &dict), true));
    ASSERT_EQ(true, patchTree->append(patch, 0)) << "Appending a patch with one elements failed";
}

TEST_F(PatchTreeTest, AppendNotNew) {
    Patch patch1(&dict);
    patch1.add(PatchElement(Triple("s2", "p2", "o2", &dict), true));

    Patch patch2(&dict);
    patch2.add(PatchElement(Triple("s1", "p1", "o1", &dict), true));
    patch2.add(PatchElement(Triple("s2", "p2", "o2", &dict), true));
    patch2.add(PatchElement(Triple("s3", "p3", "o3", &dict), true));

    Patch patch3(&dict);
    patch3.add(PatchElement(Triple("s2", "p2", "o2", &dict), false));

    ASSERT_EQ(true, patchTree->append(patch1, 0)) << "Appending a patch with one elements failed";
    ASSERT_EQ(false, patchTree->append(patch2, 0)) << "Appending a patch with 3 elements succeeded where it should have failed";

    ASSERT_EQ(false, patchTree->contains(PatchElement(Triple("s1", "p1", "o1", &dict), true), 0, false)) << "Failing to append a patch should not have (some) of its element added.";
    ASSERT_EQ(true , patchTree->contains(PatchElement(Triple("s2", "p2", "o2", &dict), true), 0, false)) << "Failing to append a patch should not have (some) of its element added.";
    ASSERT_EQ(false, patchTree->contains(PatchElement(Triple("s3", "p3", "o3", &dict), true), 0, false)) << "Failing to append a patch should not have (some) of its element added.";

    ASSERT_EQ(false, patchTree->append(patch3, 0)) << "Appending a patch with one elements succeeded where it should have failed";
}

TEST_F(PatchTreeTest, AppendRemove) {
    Patch patch1(&dict);
    for(int i = 0; i < 1000; i++) {
        string e = std::to_string(i);
        patch1.add(PatchElement(Triple("b" + e, "b" + e, "b" + e, &dict), true));
    }
    ASSERT_EQ(true, patchTree->append(patch1, 0)) << "Appending a patch failed at an iteration";

    Patch patch2(&dict);
    for(int i = 0; i < 1000; i++) {
        string e = std::to_string(i * 20);
        patch2.add(PatchElement(Triple(e, e, e, &dict), false));
    }
    ASSERT_EQ(true, patchTree->append(patch2, 1)) << "Appending a patch failed at an iteration";

    Patch patch3(&dict);
    for(int i = 0; i < 1000; i++) {
        string e = std::to_string(i * 3);
        patch3.add(PatchElement(Triple("b" + e, "b" + e, "b" + e, &dict), false));
    }
    ASSERT_EQ(true, patchTree->append(patch3, 2)) << "Appending a patch failed at an iteration";

    for(int i = 0; i < 1000; i++) {
        string e = std::to_string(i);
        ASSERT_EQ(true, patchTree->contains(PatchElement(Triple("b" + e, "b" + e, "b" + e, &dict), true), 0, false)) << "Failing to append a patch should not have (some) of its element added.";
        ASSERT_EQ(false, patchTree->contains(PatchElement(Triple("b" + e, "b" + e, "b" + e, &dict), false), 0, false)) << "Failing to append a patch should not have (some) of its element added.";
    }
}

TEST_F(PatchTreeTest, RepeatAppendRemove1) {
    for(int i = 0; i < 10; i++) {
        Patch patch(&dict);
        patch.add(PatchElement(Triple("a", "a", "a", &dict), i % 2));
        ASSERT_EQ(true, patchTree->append(patch, i)) << "Appending a patch failed at an iteration";
    }

    ASSERT_EQ("a a a. (-)\n", patchTree->reconstruct_patch(0).to_string(dict));
    ASSERT_EQ("a a a. (+) L\n", patchTree->reconstruct_patch(1).to_string(dict));
    ASSERT_EQ("a a a. (-)\n", patchTree->reconstruct_patch(2).to_string(dict));
    ASSERT_EQ("a a a. (+) L\n", patchTree->reconstruct_patch(3).to_string(dict));
    ASSERT_EQ("a a a. (-)\n", patchTree->reconstruct_patch(4).to_string(dict));
    ASSERT_EQ("a a a. (+) L\n", patchTree->reconstruct_patch(5).to_string(dict));
    ASSERT_EQ("a a a. (-)\n", patchTree->reconstruct_patch(6).to_string(dict));
    ASSERT_EQ("a a a. (+) L\n", patchTree->reconstruct_patch(7).to_string(dict));
    ASSERT_EQ("a a a. (-)\n", patchTree->reconstruct_patch(8).to_string(dict));
    ASSERT_EQ("a a a. (+) L\n", patchTree->reconstruct_patch(9).to_string(dict));
    ASSERT_EQ("a a a. (+) L\n", patchTree->reconstruct_patch(10).to_string(dict));
}

TEST_F(PatchTreeTest, RepeatAppendRemove2) {
    for(int i = 0; i < 10; i++) {
        Patch patch(&dict);
        patch.add(PatchElement(Triple("a", "a", "a", &dict), !(i % 2)));
        ASSERT_EQ(true, patchTree->append(patch, i)) << "Appending a patch failed at an iteration";
    }

    ASSERT_EQ("a a a. (+)\n", patchTree->reconstruct_patch(0).to_string(dict));
    ASSERT_EQ("a a a. (-) L\n", patchTree->reconstruct_patch(1).to_string(dict));
    ASSERT_EQ("a a a. (+)\n", patchTree->reconstruct_patch(2).to_string(dict));
    ASSERT_EQ("a a a. (-) L\n", patchTree->reconstruct_patch(3).to_string(dict));
    ASSERT_EQ("a a a. (+)\n", patchTree->reconstruct_patch(4).to_string(dict));
    ASSERT_EQ("a a a. (-) L\n", patchTree->reconstruct_patch(5).to_string(dict));
    ASSERT_EQ("a a a. (+)\n", patchTree->reconstruct_patch(6).to_string(dict));
    ASSERT_EQ("a a a. (-) L\n", patchTree->reconstruct_patch(7).to_string(dict));
    ASSERT_EQ("a a a. (+)\n", patchTree->reconstruct_patch(8).to_string(dict));
    ASSERT_EQ("a a a. (-) L\n", patchTree->reconstruct_patch(9).to_string(dict));
    ASSERT_EQ("a a a. (-) L\n", patchTree->reconstruct_patch(10).to_string(dict));
}

TEST_F(PatchTreeTest, IteratorOrder) {
    Patch patch(&dict);
    patch.add(PatchElement(Triple("g", "p", "o", &dict), false));
    patch.add(PatchElement(Triple("a", "p", "o", &dict), true));
    patch.add(PatchElement(Triple("s", "z", "o", &dict), false));
    patch.add(PatchElement(Triple("s", "a", "o", &dict), true));
    // Expected order:
    // g p o -
    // s z o -
    // a p o +
    // s a o +
    patchTree->append_unsafe(patch, 0);

    PatchTreeKey iteratorKey = Triple("a", "a", "a", &dict);
    PatchTreeIterator it = patchTree->iterator(&iteratorKey); // Iterate starting from the given triple.
    PatchTreeKey key;
    PatchTreeValue value;

    ASSERT_EQ(true, it.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("a p o.", key.to_string(dict)) << "First key is incorrect";
    ASSERT_EQ(true, value.is_addition(0, true)) << "Found value is incorrect";
    ASSERT_EQ(true, value.get_addition()->is_patch_id(0)) << "Found value is incorrect";

    ASSERT_EQ(true, it.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("g p o.", key.to_string(dict)) << "Second key is incorrect";
    ASSERT_EQ(true, value.is_deletion(0, true)) << "Second value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(0).get_patch_id()) << "Second value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(0).get_patch_positions().sp_) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(0).get_patch_positions().s_o) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(0).get_patch_positions().s__) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(0).get_patch_positions()._po) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(0).get_patch_positions()._p_) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(0).get_patch_positions().__o) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(0).get_patch_positions().___) << "Found value is incorrect";

    ASSERT_EQ(true, it.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", key.to_string(dict)) << "Third key is incorrect";
    ASSERT_EQ(true, value.is_addition(0, true)) << "Third value is incorrect";
    ASSERT_EQ(true, value.get_addition()->is_patch_id(0)) << "Third value is incorrect";

    ASSERT_EQ(true, it.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("s z o.", key.to_string(dict)) << "Fourth key is incorrect";
    ASSERT_EQ(true, value.is_deletion(0, true)) << "Fourth value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(0).get_patch_id()) << "Fourth value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(0).get_patch_positions().sp_) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(0).get_patch_positions().s_o) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(0).get_patch_positions().s__) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(0).get_patch_positions()._po) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(0).get_patch_positions()._p_) << "Found value is incorrect";
    ASSERT_EQ(1, value.get_deletion()->get(0).get_patch_positions().__o) << "Found value is incorrect";
    ASSERT_EQ(1, value.get_deletion()->get(0).get_patch_positions().___) << "Found value is incorrect";

    ASSERT_EQ(false, it.next(&key, &value)) << "Iterator should be finished";
}

TEST_F(PatchTreeTest, PatchIterator) {
    Patch patch1(&dict);
    patch1.add(PatchElement(Triple("g", "p", "o", &dict), false));
    patch1.add(PatchElement(Triple("a", "p", "o", &dict), true));
    patch1.add(PatchElement(Triple("s", "z", "o", &dict), false));
    patch1.add(PatchElement(Triple("s", "a", "o", &dict), true));
    patchTree->append_unsafe(patch1, 1);

    Patch patch2(&dict);
    patch2.add(PatchElement(Triple("q", "p", "o", &dict), false));
    patch2.add(PatchElement(Triple("g", "p", "o", &dict), true));
    patch2.add(PatchElement(Triple("s", "z", "o", &dict), false));
    patch2.add(PatchElement(Triple("s", "a", "o", &dict), true));
    patchTree->append_unsafe(patch2, 2);

    Patch patch3(&dict);
    patch3.add(PatchElement(Triple("g", "p", "o", &dict), false));
    patch3.add(PatchElement(Triple("a", "p", "o", &dict), false));
    patch3.add(PatchElement(Triple("h", "z", "o", &dict), false));
    patch3.add(PatchElement(Triple("l", "a", "o", &dict), true));
    patchTree->append_unsafe(patch3, 3);

    PatchTreeKey iteratorKey = Triple("s", "a", "o", &dict);
    PatchTreeIterator it = patchTree->iterator(&iteratorKey, 2, false); // Iterate over all elements of patch 2 starting from "s a o."
    PatchTreeKey key;
    PatchTreeValue value;

    ASSERT_EQ(true, it.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", key.to_string(dict)) << "Fourth key is incorrect";
    ASSERT_EQ(true, value.is_addition(2, true)) << "Fourth value is incorrect";

    ASSERT_EQ(true, it.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("s z o.", key.to_string(dict)) << "Third key is incorrect";
    ASSERT_EQ(false, value.is_addition(2, true)) << "Third value is incorrect";

    ASSERT_EQ(false, it.next(&key, &value)) << "Iterator should be finished";
}

TEST_F(PatchTreeTest, OffsetFilteredPatchIterator) {
    Patch patch1(&dict);
    patch1.add(PatchElement(Triple("g", "p", "o", &dict), false));
    patch1.add(PatchElement(Triple("a", "p", "o", &dict), true));
    patch1.add(PatchElement(Triple("s", "z", "o", &dict), false));
    patch1.add(PatchElement(Triple("s", "a", "o", &dict), true));
    patchTree->append(patch1, 1);

    Patch patch2(&dict);
    patch2.add(PatchElement(Triple("q", "p", "o", &dict), false));
    patch2.add(PatchElement(Triple("g", "p", "o", &dict), true));
    patch2.add(PatchElement(Triple("s", "z", "o", &dict), false));
    patch2.add(PatchElement(Triple("s", "a", "o", &dict), true));
    patchTree->append(patch2, 2);

    Patch patch3(&dict);
    patch3.add(PatchElement(Triple("g", "p", "o", &dict), false));
    patch3.add(PatchElement(Triple("a", "p", "o", &dict), false));
    patch3.add(PatchElement(Triple("h", "z", "o", &dict), false));
    patch3.add(PatchElement(Triple("l", "a", "o", &dict), true));
    patchTree->append(patch3, 3);

    // Expected order for patch 1:
    // a p o +
    // g p o -
    // s a o +
    // s z o -

    // Expected order for patch 2:
    // a p o +
    // g p o + L
    // q p o -
    // s a o +
    // s z o -

    PatchTreeIterator it = patchTree->iterator(2, false); // Iterate over all elements of patch only 2
    PatchTreeKey key;
    PatchTreeValue value;

    ASSERT_EQ(true, it.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("a p o.", key.to_string(dict)) << "First key is incorrect";
    ASSERT_EQ(true, value.is_addition(2, true)) << "First value is incorrect";

    ASSERT_EQ(true, it.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("g p o.", key.to_string(dict)) << "First key is incorrect";
    ASSERT_EQ(true, value.is_addition(2, true)) << "First value is incorrect";

    ASSERT_EQ(true, it.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("q p o.", key.to_string(dict)) << "Second key is incorrect";
    ASSERT_EQ(true, value.is_deletion(2, true)) << "Second value is incorrect";

    ASSERT_EQ(true, it.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", key.to_string(dict)) << "Third key is incorrect";
    ASSERT_EQ(true, value.is_addition(2, true)) << "Third value is incorrect";

    ASSERT_EQ(true, it.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("s z o.", key.to_string(dict)) << "Fourth key is incorrect";
    ASSERT_EQ(true, value.is_deletion(2, true)) << "Fourth value is incorrect";

    ASSERT_EQ(false, it.next(&key, &value)) << "Iterator should be finished";

    PatchTreeIterator it2 = patchTree->iterator(2, false); // Iterate over all elements of patch 2 and before

    ASSERT_EQ(true, it2.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("a p o.", key.to_string(dict)) << "First key is incorrect";
    ASSERT_EQ(true, value.is_addition(2, true)) << "First value is incorrect";

    ASSERT_EQ(true, it2.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("g p o.", key.to_string(dict)) << "First key is incorrect";
    ASSERT_EQ(true, value.is_addition(2, true)) << "First value is incorrect";

    ASSERT_EQ(true, it2.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("q p o.", key.to_string(dict)) << "Second key is incorrect";
    ASSERT_EQ(false, value.is_addition(2, true)) << "Second value is incorrect";

    ASSERT_EQ(true, it2.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", key.to_string(dict)) << "Third key is incorrect";
    ASSERT_EQ(true, value.is_addition(2, true)) << "Third value is incorrect";

    ASSERT_EQ(true, it2.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("s z o.", key.to_string(dict)) << "Fourth key is incorrect";
    ASSERT_EQ(true, value.is_deletion(2, true)) << "Fourth value is incorrect";

    ASSERT_EQ(false, it2.next(&key, &value)) << "Iterator should be finished";
}

TEST_F(PatchTreeTest, ReconstructPatchSimple1) {
    Patch patch1(&dict);
    patch1.add(PatchElement(Triple("g", "p", "o", &dict), false));
    patch1.add(PatchElement(Triple("a", "p", "o", &dict), true));
    patchTree->append_unsafe(patch1, 1);

    Patch patch2_copy = patchTree->reconstruct_patch(1);
    ASSERT_EQ("a p o. (+)\n"
              "g p o. (-)\n", patch2_copy.to_string(dict)) << "Reconstructed patch should be equal to inserted patch";
}

TEST_F(PatchTreeTest, ReconstructPatchSimple2) {
    Patch patch1(&dict);
    patch1.add(PatchElement(Triple("g", "p", "o", &dict), false));
    patch1.add(PatchElement(Triple("a", "p", "o", &dict), true));
    patchTree->append_unsafe(patch1, 1);

    Patch patch2_copy = patchTree->reconstruct_patch(2);
    ASSERT_EQ("a p o. (+)\n"
              "g p o. (-)\n", patch2_copy.to_string(dict)) << "Reconstructed patch should be equal to inserted patch";
}

TEST_F(PatchTreeTest, ReconstructPatchSimple3) {
    Patch patch1(&dict);
    patch1.add(PatchElement(Triple("g", "p", "o", &dict), false));
    patch1.add(PatchElement(Triple("a", "p", "o", &dict), true));
    patchTree->append_unsafe(patch1, 1);

    Patch patch2(&dict);
    patch2.add(PatchElement(Triple("q", "p", "o", &dict), false));
    patch2.add(PatchElement(Triple("g", "p", "o", &dict), true));
    patchTree->append_unsafe(patch2, 2);

    Patch patch2_copy = patchTree->reconstruct_patch(2);
    ASSERT_EQ("a p o. (+)\n"
              "g p o. (+) L\n"
              "q p o. (-)\n", patch2_copy.to_string(dict)) << "Reconstructed patch should be equal to inserted patch";
}

TEST_F(PatchTreeTest, ReconstructPatchSimple4) {
    Patch patch1(&dict);
    patch1.add(PatchElement(Triple("g", "p", "o", &dict), true));
    patch1.add(PatchElement(Triple("a", "p", "o", &dict), true));
    patchTree->append_unsafe(patch1, 1);

    Patch patch2(&dict);
    patch2.add(PatchElement(Triple("q", "p", "o", &dict), false));
    patch2.add(PatchElement(Triple("g", "p", "o", &dict), false));
    patchTree->append_unsafe(patch2, 2);

    Patch patch2_copy = patchTree->reconstruct_patch(2);
    ASSERT_EQ("a p o. (+)\n"
              "g p o. (-) L\n"
              "q p o. (-)\n", patch2_copy.to_string(dict)) << "Reconstructed patch should be equal to inserted patch";
}

TEST_F(PatchTreeTest, ReconstructPatchSimple5) {
    Patch patch1(&dict);
    patch1.add(PatchElement(Triple("g", "p", "o", &dict), true));
    patch1.add(PatchElement(Triple("a", "p", "o", &dict), true));
    patchTree->append_unsafe(patch1, 1);

    Patch patch2(&dict);
    patch2.add(PatchElement(Triple("q", "p", "o", &dict), false));
    patch2.add(PatchElement(Triple("g", "p", "o", &dict), false));
    patchTree->append_unsafe(patch2, 2);

    Patch patch3(&dict);
    patch3.add(PatchElement(Triple("g", "p", "o", &dict), true));
    patch3.add(PatchElement(Triple("a", "p", "o", &dict), false));
    patchTree->append_unsafe(patch3, 3);

    Patch patch2_copy = patchTree->reconstruct_patch(2);
    ASSERT_EQ("a p o. (+)\n"
              "g p o. (-) L\n"
              "q p o. (-)\n", patch2_copy.to_string(dict)) << "Reconstructed patch should be equal to inserted patch";
}

TEST_F(PatchTreeTest, ReconstructPatchSimple6) {
    Patch patch1(&dict);
    patch1.add(PatchElement(Triple("g", "p", "o", &dict), false));
    patch1.add(PatchElement(Triple("a", "p", "o", &dict), true));
    patchTree->append_unsafe(patch1, 1);

    Patch patch2(&dict);
    patch2.add(PatchElement(Triple("q", "p", "o", &dict), false));
    patch2.add(PatchElement(Triple("g", "p", "o", &dict), true));
    patchTree->append_unsafe(patch2, 2);

    Patch patch3(&dict);
    patch3.add(PatchElement(Triple("g", "p", "o", &dict), false));
    patch3.add(PatchElement(Triple("a", "p", "o", &dict), false));
    patchTree->append_unsafe(patch3, 3);

    Patch patch2_copy = patchTree->reconstruct_patch(2);
    ASSERT_EQ("a p o. (+)\n"
              "g p o. (+) L\n"
              "q p o. (-)\n", patch2_copy.to_string(dict)) << "Reconstructed patch should be equal to inserted patch";
}

TEST_F(PatchTreeTest, ReconstructPatchSingle) {
    Patch patch1(&dict);
    patch1.add(PatchElement(Triple("g", "p", "o", &dict), false));
    patch1.add(PatchElement(Triple("a", "p", "o", &dict), true));
    patch1.add(PatchElement(Triple("s", "z", "o", &dict), false));
    patch1.add(PatchElement(Triple("s", "a", "o", &dict), true));
    patchTree->append_unsafe(patch1, 1);

    Patch patch2(&dict);
    patch2.add(PatchElement(Triple("q", "p", "o", &dict), false));
    patch2.add(PatchElement(Triple("g", "p", "o", &dict), true));
    patch2.add(PatchElement(Triple("s", "z", "o", &dict), false));
    patch2.add(PatchElement(Triple("s", "a", "o", &dict), true));
    patchTree->append_unsafe(patch2, 2);

    Patch patch3(&dict);
    patch3.add(PatchElement(Triple("g", "p", "o", &dict), false));
    patch3.add(PatchElement(Triple("a", "p", "o", &dict), false));
    patch3.add(PatchElement(Triple("h", "z", "o", &dict), false));
    patch3.add(PatchElement(Triple("l", "a", "o", &dict), true));
    patchTree->append_unsafe(patch3, 3);

    Patch patch2_copy = patchTree->reconstruct_patch(2);
    ASSERT_EQ("a p o. (+)\n"
              "g p o. (+) L\n"
              "q p o. (-)\n"
              "s a o. (+)\n"
              "s z o. (-)\n", patch2_copy.to_string(dict)) << "Reconstructed patch should be equal to inserted patch";
}

TEST_F(PatchTreeTest, ReconstructPatchComposite) {
    Patch patch1(&dict);
    patch1.add(PatchElement(Triple("g", "p", "o", &dict), false));
    patch1.add(PatchElement(Triple("a", "p", "o", &dict), true));
    patchTree->append_unsafe(patch1, 1);

    Patch patch2(&dict);
    patch2.add(PatchElement(Triple("s", "z", "o", &dict), false));
    patch2.add(PatchElement(Triple("s", "a", "o", &dict), true));
    patchTree->append_unsafe(patch2, 1);

    Patch patch3(&dict);
    patch3.add(PatchElement(Triple("g", "p", "o", &dict), false));
    patch3.add(PatchElement(Triple("a", "p", "o", &dict), false));
    patch3.add(PatchElement(Triple("h", "z", "o", &dict), false));
    patch3.add(PatchElement(Triple("l", "a", "o", &dict), true));
    patchTree->append_unsafe(patch3, 2);

    Patch patch2_copy = patchTree->reconstruct_patch(1);
    ASSERT_EQ("a p o. (+)\n"
              "g p o. (-)\n"
              "s a o. (+)\n"
              "s z o. (-)\n", patch2_copy.to_string(dict)) << "Reconstructed patch should be equal to the given patch";
}

TEST_F(PatchTreeTest, ReconstructPatchComposite2) {
    Patch patch1(&dict);
    patch1.add(PatchElement(Triple("g", "p", "o", &dict), false));
    patch1.add(PatchElement(Triple("a", "p", "o", &dict), true));
    patchTree->append(patch1, 1);

    Patch patch2(&dict);
    patch2.add(PatchElement(Triple("s", "z", "o", &dict), false));
    patch2.add(PatchElement(Triple("s", "a", "o", &dict), true));
    patchTree->append(patch2, 1);

    Patch patch3(&dict);
    patch3.add(PatchElement(Triple("g", "p", "o", &dict), false));
    patch3.add(PatchElement(Triple("a", "p", "o", &dict), false));
    patch3.add(PatchElement(Triple("h", "z", "o", &dict), false));
    patch3.add(PatchElement(Triple("l", "a", "o", &dict), true));
    patchTree->append(patch3, 2);

    Patch patch4(&dict);
    patch4.add(PatchElement(Triple("h", "p", "o", &dict), false));
    patch4.add(PatchElement(Triple("s", "z", "o", &dict), false));
    patchTree->append(patch4, 4);

    Patch patch5(&dict);
    patch5.add(PatchElement(Triple("h", "p", "o", &dict), true));
    patchTree->append(patch5, 5);

    Patch patch1_copy = patchTree->reconstruct_patch(1);
    ASSERT_EQ("a p o. (+)\n"
              "g p o. (-)\n"
              "s a o. (+)\n"
              "s z o. (-)\n", patch1_copy.to_string(dict)) << "Reconstructed patch should be equal to the given patch";

    Patch patch2_copy = patchTree->reconstruct_patch(2);
    ASSERT_EQ("a p o. (-) L\n"
              "g p o. (-)\n"
              "h z o. (-)\n"
              "l a o. (+)\n"
              "s a o. (+)\n"
              "s z o. (-)\n", patch2_copy.to_string(dict)) << "Reconstructed patch should be equal to the given patch";

    Patch patch4_copy = patchTree->reconstruct_patch(4);
    ASSERT_EQ("a p o. (-) L\n"
              "g p o. (-)\n"
              "h p o. (-)\n"
              "h z o. (-)\n"
              "l a o. (+)\n"
              "s a o. (+)\n"
              "s z o. (-)\n", patch4_copy.to_string(dict)) << "Reconstructed patch should be equal to the given patch";

    Patch patch5_copy = patchTree->reconstruct_patch(5);
    ASSERT_EQ("a p o. (-) L\n"
              "g p o. (-)\n"
              "h p o. (+) L\n"
              "h z o. (-)\n"
              "l a o. (+)\n"
              "s a o. (+)\n"
              "s z o. (-)\n", patch5_copy.to_string(dict)) << "Reconstructed patch should be equal to the given patch";
}

TEST_F(PatchTreeTest, RelativePatchPositions) {
    Patch patch1(&dict);
    patch1.add(PatchElement(Triple("g", "p", "o", &dict), false));
    patch1.add(PatchElement(Triple("a", "p", "o", &dict), false));
    patchTree->append_unsafe(patch1, 1);

    Patch patch2(&dict);
    patch2.add(PatchElement(Triple("s", "z", "o", &dict), false));
    patch2.add(PatchElement(Triple("a", "b", "c", &dict), true));
    patch2.add(PatchElement(Triple("s", "a", "o", &dict), false));
    patchTree->append_unsafe(patch2, 1);

    Patch patch3(&dict);
    patch3.add(PatchElement(Triple("g", "p", "o", &dict), false));
    patch3.add(PatchElement(Triple("a", "p", "o", &dict), false));
    patch3.add(PatchElement(Triple("h", "z", "o", &dict), false));
    patch3.add(PatchElement(Triple("l", "a", "o", &dict), true));
    patchTree->append_unsafe(patch3, 2);

    Patch patch4(&dict);
    patch4.add(PatchElement(Triple("h", "p", "o", &dict), false));
    patch4.add(PatchElement(Triple("s", "z", "o", &dict), false));
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
    ASSERT_EQ("a b c.", key.to_string(dict)) << "First key is incorrect";
    ASSERT_EQ(true, value.is_addition(1, true)) << "Found value is incorrect";

    ASSERT_EQ(true, it.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("a p o.", key.to_string(dict)) << "First key is incorrect";
    ASSERT_EQ(true, value.is_deletion(1, true)) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(1).get_patch_positions().sp_) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(1).get_patch_positions().s_o) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(1).get_patch_positions().s__) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(1).get_patch_positions()._po) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(1).get_patch_positions()._p_) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(1).get_patch_positions().__o) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(1).get_patch_positions().___) << "Found value is incorrect";

    ASSERT_EQ(true, it.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("g p o.", key.to_string(dict)) << "Second key is incorrect";
    ASSERT_EQ(true, value.is_deletion(1, true)) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(1).get_patch_positions().sp_) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(1).get_patch_positions().s_o) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(1).get_patch_positions().s__) << "Found value is incorrect";
    ASSERT_EQ(1, value.get_deletion()->get(1).get_patch_positions()._po) << "Found value is incorrect";
    ASSERT_EQ(1, value.get_deletion()->get(1).get_patch_positions()._p_) << "Found value is incorrect";
    ASSERT_EQ(1, value.get_deletion()->get(1).get_patch_positions().__o) << "Found value is incorrect";
    ASSERT_EQ(1, value.get_deletion()->get(1).get_patch_positions().___) << "Found value is incorrect";

    ASSERT_EQ(true, it.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", key.to_string(dict)) << "Third key is incorrect";
    ASSERT_EQ(true, value.is_deletion(1, true)) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(1).get_patch_positions().sp_) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(1).get_patch_positions().s_o) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(1).get_patch_positions().s__) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(1).get_patch_positions()._po) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(1).get_patch_positions()._p_) << "Found value is incorrect";
    ASSERT_EQ(2, value.get_deletion()->get(1).get_patch_positions().__o) << "Found value is incorrect";
    ASSERT_EQ(2, value.get_deletion()->get(1).get_patch_positions().___) << "Found value is incorrect";

    ASSERT_EQ(true, it.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("s z o.", key.to_string(dict)) << "Fourth key is incorrect";
    ASSERT_EQ(true, value.is_deletion(1, true)) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(1).get_patch_positions().sp_) << "Found value is incorrect";
    ASSERT_EQ(1, value.get_deletion()->get(1).get_patch_positions().s_o) << "Found value is incorrect";
    ASSERT_EQ(1, value.get_deletion()->get(1).get_patch_positions().s__) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(1).get_patch_positions()._po) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(1).get_patch_positions()._p_) << "Found value is incorrect";
    ASSERT_EQ(3, value.get_deletion()->get(1).get_patch_positions().__o) << "Found value is incorrect";
    ASSERT_EQ(3, value.get_deletion()->get(1).get_patch_positions().___) << "Found value is incorrect";

    ASSERT_EQ(false, it.next(&key, &value)) << "Iterator should be finished";

    Patch patch5(&dict);
    patch5.add(PatchElement(Triple("a", "a", "a", &dict), false));
    patch5.add(PatchElement(Triple("z", "z", "z", &dict), false));
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
    ASSERT_EQ("a a a.", key.to_string(dict)) << "First key is incorrect";
    ASSERT_EQ(true, value.is_deletion(1, true)) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(1).get_patch_positions().sp_) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(1).get_patch_positions().s_o) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(1).get_patch_positions().s__) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(1).get_patch_positions()._po) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(1).get_patch_positions()._p_) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(1).get_patch_positions().__o) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(1).get_patch_positions().___) << "Found value is incorrect";

    ASSERT_EQ(true, it2.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("a b c.", key.to_string(dict)) << "First key is incorrect";
    ASSERT_EQ(true, value.is_addition(1, true)) << "Found value is incorrect";

    ASSERT_EQ(true, it2.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("a p o.", key.to_string(dict)) << "Second key is incorrect";
    ASSERT_EQ(true, value.is_deletion(1, true)) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(1).get_patch_positions().sp_) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(1).get_patch_positions().s_o) << "Found value is incorrect";
    ASSERT_EQ(1, value.get_deletion()->get(1).get_patch_positions().s__) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(1).get_patch_positions()._po) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(1).get_patch_positions()._p_) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(1).get_patch_positions().__o) << "Found value is incorrect";
    ASSERT_EQ(1, value.get_deletion()->get(1).get_patch_positions().___) << "Found value is incorrect";

    ASSERT_EQ(true, it2.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("g p o.", key.to_string(dict)) << "Third key is incorrect";
    ASSERT_EQ(true, value.is_deletion(1, true)) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(1).get_patch_positions().sp_) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(1).get_patch_positions().s_o) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(1).get_patch_positions().s__) << "Found value is incorrect";
    ASSERT_EQ(1, value.get_deletion()->get(1).get_patch_positions()._po) << "Found value is incorrect";
    ASSERT_EQ(1, value.get_deletion()->get(1).get_patch_positions()._p_) << "Found value is incorrect";
    ASSERT_EQ(1, value.get_deletion()->get(1).get_patch_positions().__o) << "Found value is incorrect";
    ASSERT_EQ(2, value.get_deletion()->get(1).get_patch_positions().___) << "Found value is incorrect";

    ASSERT_EQ(true, it2.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", key.to_string(dict)) << "Fourth key is incorrect";
    ASSERT_EQ(true, value.is_deletion(1, true)) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(1).get_patch_positions().sp_) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(1).get_patch_positions().s_o) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(1).get_patch_positions().s__) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(1).get_patch_positions()._po) << "Found value is incorrect";
    ASSERT_EQ(1, value.get_deletion()->get(1).get_patch_positions()._p_) << "Found value is incorrect";
    ASSERT_EQ(2, value.get_deletion()->get(1).get_patch_positions().__o) << "Found value is incorrect";
    ASSERT_EQ(3, value.get_deletion()->get(1).get_patch_positions().___) << "Found value is incorrect";

    ASSERT_EQ(true, it2.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("s z o.", key.to_string(dict)) << "Fifth key is incorrect";
    ASSERT_EQ(true, value.is_deletion(1, true)) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(1).get_patch_positions().sp_) << "Found value is incorrect";
    ASSERT_EQ(1, value.get_deletion()->get(1).get_patch_positions().s_o) << "Found value is incorrect";
    ASSERT_EQ(1, value.get_deletion()->get(1).get_patch_positions().s__) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(1).get_patch_positions()._po) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(1).get_patch_positions()._p_) << "Found value is incorrect";
    ASSERT_EQ(3, value.get_deletion()->get(1).get_patch_positions().__o) << "Found value is incorrect";
    ASSERT_EQ(4, value.get_deletion()->get(1).get_patch_positions().___) << "Found value is incorrect";

    ASSERT_EQ(true, it2.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("z z z.", key.to_string(dict)) << "Sixth key is incorrect";
    ASSERT_EQ(true, value.is_deletion(1, true)) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(1).get_patch_positions().sp_) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(1).get_patch_positions().s_o) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(1).get_patch_positions().s__) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(1).get_patch_positions()._po) << "Found value is incorrect";
    ASSERT_EQ(1, value.get_deletion()->get(1).get_patch_positions()._p_) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(1).get_patch_positions().__o) << "Found value is incorrect";
    ASSERT_EQ(5, value.get_deletion()->get(1).get_patch_positions().___) << "Found value is incorrect";

    ASSERT_EQ(false, it2.next(&key, &value)) << "Iterator should be finished";
}

TEST_F(PatchTreeTest, RelativePatchPositions2) {
    Patch patch1(&dict);
    patch1.add(PatchElement(Triple("a", "b", "c", &dict), true));
    patch1.add(PatchElement(Triple("a", "a", "a", &dict), false));
    patch1.add(PatchElement(Triple("z", "z", "z", &dict), false));
    patchTree->append_unsafe(patch1, 1);

    Patch patch2(&dict);
    patch2.add(PatchElement(Triple("a", "b", "c", &dict), false));
    patchTree->append_unsafe(patch2, 2);

    PatchTreeIterator it1 = patchTree->iterator(1, false); // Iterate over all elements of patch 1
    PatchTreeKey key;
    PatchTreeValue value;

    // Expected order for patch 1:
    // a a a -
    // a b c +
    // z z z -

    ASSERT_EQ(true, it1.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("a a a.", key.to_string(dict)) << "Key is incorrect";
    ASSERT_EQ(true, value.is_deletion(1, true)) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(1).get_patch_positions().sp_) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(1).get_patch_positions().s_o) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(1).get_patch_positions().s__) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(1).get_patch_positions()._po) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(1).get_patch_positions()._p_) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(1).get_patch_positions().__o) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(1).get_patch_positions().___) << "Found value is incorrect";

    ASSERT_EQ(true, it1.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("a b c.", key.to_string(dict)) << "Key is incorrect";
    ASSERT_EQ(true, value.is_addition(1, true)) << "Found value is incorrect";

    ASSERT_EQ(true, it1.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("z z z.", key.to_string(dict)) << "Key is incorrect";
    ASSERT_EQ(true, value.is_deletion(1, true)) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(1).get_patch_positions().sp_) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(1).get_patch_positions().s_o) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(1).get_patch_positions().s__) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(1).get_patch_positions()._po) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(1).get_patch_positions()._p_) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(1).get_patch_positions().__o) << "Found value is incorrect";
    ASSERT_EQ(1, value.get_deletion()->get(1).get_patch_positions().___) << "Found value is incorrect";

    ASSERT_EQ(false, it1.next(&key, &value)) << "Iterator should be finished";

    PatchTreeIterator it2 = patchTree->iterator(2, false); // Iterate over all elements of patch 2
    // Expected order for patch 2:
    // a a a -
    // a b c -
    // z z z -

    ASSERT_EQ(true, it2.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("a a a.", key.to_string(dict)) << "Key is incorrect";
    ASSERT_EQ(true, value.is_deletion(2, true)) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(2).get_patch_positions().sp_) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(2).get_patch_positions().s_o) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(2).get_patch_positions().s__) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(2).get_patch_positions()._po) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(2).get_patch_positions()._p_) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(2).get_patch_positions().__o) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(2).get_patch_positions().___) << "Found value is incorrect";

    ASSERT_EQ(true, it2.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("a b c.", key.to_string(dict)) << "Key is incorrect";
    ASSERT_EQ(true, value.is_deletion(2, true)) << "Found value is incorrect";
    // This is a local change, so all patch positions are -1 because they are not applicable.
    ASSERT_EQ(-1, value.get_deletion()->get(2).get_patch_positions().sp_) << "Found value is incorrect";
    ASSERT_EQ(-1, value.get_deletion()->get(2).get_patch_positions().s_o) << "Found value is incorrect";
    ASSERT_EQ(-1, value.get_deletion()->get(2).get_patch_positions().s__) << "Found value is incorrect";
    ASSERT_EQ(-1, value.get_deletion()->get(2).get_patch_positions()._po) << "Found value is incorrect";
    ASSERT_EQ(-1, value.get_deletion()->get(2).get_patch_positions()._p_) << "Found value is incorrect";
    ASSERT_EQ(-1, value.get_deletion()->get(2).get_patch_positions().__o) << "Found value is incorrect";
    ASSERT_EQ(-1, value.get_deletion()->get(2).get_patch_positions().___) << "Found value is incorrect";

    ASSERT_EQ(true, it2.next(&key, &value)) << "Iterator has a no next value";
    ASSERT_EQ("z z z.", key.to_string(dict)) << "Key is incorrect";
    ASSERT_EQ(true, value.is_deletion(2, true)) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(2).get_patch_positions().sp_) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(2).get_patch_positions().s_o) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(2).get_patch_positions().s__) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(2).get_patch_positions()._po) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(2).get_patch_positions()._p_) << "Found value is incorrect";
    ASSERT_EQ(0, value.get_deletion()->get(2).get_patch_positions().__o) << "Found value is incorrect";
    ASSERT_EQ(1, value.get_deletion()->get(2).get_patch_positions().___) << "Found value is incorrect";

    ASSERT_EQ(false, it2.next(&key, &value)) << "Iterator should be finished";
}

TEST_F(PatchTreeTest, DeletionCount) {
    Patch patch1(&dict);
    patch1.add(PatchElement(Triple("g", "p", "o", &dict), false));
    patch1.add(PatchElement(Triple("a", "p", "o", &dict), true));
    patchTree->append(patch1, 1);

    Patch patch2(&dict);
    patch2.add(PatchElement(Triple("s", "z", "o", &dict), false));
    patch2.add(PatchElement(Triple("s", "a", "o", &dict), true));
    patchTree->append(patch2, 1);

    Patch patch3(&dict);
    patch3.add(PatchElement(Triple("g", "p", "o", &dict), false));
    patch3.add(PatchElement(Triple("a", "p", "o", &dict), false));
    patch3.add(PatchElement(Triple("h", "z", "o", &dict), false));
    patch3.add(PatchElement(Triple("l", "a", "o", &dict), true));
    patchTree->append(patch3, 2);

    Patch patch4(&dict);
    patch4.add(PatchElement(Triple("h", "p", "o", &dict), false));
    patch4.add(PatchElement(Triple("s", "z", "o", &dict), false));
    patchTree->append(patch4, 4);

    // Patch 1
    // a p o +
    // g p o -
    // s a o +
    // s z o -

    ASSERT_EQ(2, patchTree->deletion_count(Triple("", "", "", &dict), 1).first) << "Deletion count is incorrect";
    ASSERT_EQ(1, patchTree->deletion_count(Triple("s", "", "", &dict), 1).first) << "Deletion count is incorrect";
    ASSERT_EQ(0, patchTree->deletion_count(Triple("s", "a", "", &dict), 1).first) << "Deletion count is incorrect";
    ASSERT_EQ(2, patchTree->deletion_count(Triple("", "", "o", &dict), 1).first) << "Deletion count is incorrect";
    ASSERT_EQ(1, patchTree->deletion_count(Triple("", "p", "o", &dict), 1).first) << "Deletion count is incorrect";
    ASSERT_EQ(1, patchTree->deletion_count(Triple("", "p", "", &dict), 1).first) << "Deletion count is incorrect";
    ASSERT_EQ(1, patchTree->deletion_count(Triple("g", "p", "", &dict), 1).first) << "Deletion count is incorrect";

    // Patch 2
    // a p o +/-
    // g p o -/- (=> ignored in count)
    // h z o -
    // l a o +
    // s a o +
    // s z o -

    ASSERT_EQ(3, patchTree->deletion_count(Triple("", "", "", &dict), 2).first) << "Deletion count is incorrect";

    // Patch 4
    // a p o +/-
    // g p o -/- (=> ignored in count)
    // h p o -
    // h z o -
    // l a o +
    // s a o +
    // s z o -/-

    ASSERT_EQ(4, patchTree->deletion_count(Triple("", "", "", &dict), 4).first) << "Deletion count is incorrect";
}

TEST_F(PatchTreeTest, DeletionIterator) {
    Patch patch1(&dict);
    patch1.add(PatchElement(Triple("g", "p", "o", &dict), false));
    patch1.add(PatchElement(Triple("a", "p", "o", &dict), true));
    patchTree->append(patch1, 1);

    Patch patch2(&dict);
    patch2.add(PatchElement(Triple("s", "z", "o", &dict), false));
    patch2.add(PatchElement(Triple("s", "a", "o", &dict), true));
    patchTree->append(patch2, 1);

    Patch patch3(&dict);
    patch3.add(PatchElement(Triple("g", "p", "o", &dict), false));
    patch3.add(PatchElement(Triple("a", "p", "o", &dict), false));
    patch3.add(PatchElement(Triple("h", "z", "o", &dict), false));
    patch3.add(PatchElement(Triple("l", "a", "o", &dict), true));
    patchTree->append(patch3, 2);

    Patch patch4(&dict);
    patch4.add(PatchElement(Triple("h", "p", "o", &dict), false));
    patch4.add(PatchElement(Triple("s", "z", "o", &dict), false));
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
    PositionedTripleIterator it1 = *patchTree->deletion_iterator_from(Triple("", "", "", &dict), 1, Triple("", "", "", &dict));

    ASSERT_EQ(true, it1.next(&pt)) << "Iterator has a no next value";
    ASSERT_EQ("g p o.", pt.triple.to_string(dict)) << "Element is incorrect";
    ASSERT_EQ(0, pt.position) << "Element is incorrect";

    ASSERT_EQ(true, it1.next(&pt)) << "Iterator has a no next value";
    ASSERT_EQ("s z o.", pt.triple.to_string(dict)) << "Element is incorrect";
    ASSERT_EQ(1, pt.position) << "Element is incorrect";

    ASSERT_EQ(false, it1.next(&pt)) << "Iterator should be finished";

    /*
     * Looping over all deletions in patch 1 starting from s z o
     */
    PositionedTripleIterator it2 = *patchTree->deletion_iterator_from(Triple("s", "z", "o", &dict), 1, Triple("", "", "", &dict));

    ASSERT_EQ(true, it2.next(&pt)) << "Iterator has a no next value";
    ASSERT_EQ("s z o.", pt.triple.to_string(dict)) << "Element is incorrect";
    ASSERT_EQ(1, pt.position) << "Element is incorrect";

    /*
     * Looping over ? z ? deletions in patch 1 starting from s z o
     */
    PositionedTripleIterator it3 = *patchTree->deletion_iterator_from(Triple("s", "z", "o", &dict), 1, Triple("", "z", "", &dict));

    ASSERT_EQ(true, it3.next(&pt)) << "Iterator has a no next value";
    ASSERT_EQ("s z o.", pt.triple.to_string(dict)) << "Element is incorrect";
    ASSERT_EQ(0, pt.position) << "Element is incorrect";

    ASSERT_EQ(false, it3.next(&pt)) << "Iterator should be finished";

    /*
     * Looping over ? ? o deletions in patch 1 starting from a non-existing start element that lies before all other elements
     */
    PositionedTripleIterator it4 = *patchTree->deletion_iterator_from(Triple("b", "b", "b", &dict), 1, Triple("", "", "o", &dict));

    ASSERT_EQ(true, it4.next(&pt)) << "Iterator has a no next value";
    ASSERT_EQ("g p o.", pt.triple.to_string(dict)) << "Element is incorrect";
    ASSERT_EQ(0, pt.position) << "Element is incorrect";

    ASSERT_EQ(true, it4.next(&pt)) << "Iterator has a no next value";
    ASSERT_EQ("s z o.", pt.triple.to_string(dict)) << "Element is incorrect";
    ASSERT_EQ(1, pt.position) << "Element is incorrect";

    ASSERT_EQ(false, it4.next(&pt)) << "Iterator should be finished";

    /*
     * Looping over ? ? o deletions in patch 1 starting from a non-existing start element that lies between the two elements
     */
    PositionedTripleIterator it5 = *patchTree->deletion_iterator_from(Triple("h", "h", "h", &dict), 1, Triple("", "", "o", &dict));

    ASSERT_EQ(true, it5.next(&pt)) << "Iterator has a no next value";
    ASSERT_EQ("s z o.", pt.triple.to_string(dict)) << "Element is incorrect";
    ASSERT_EQ(1, pt.position) << "Element is incorrect";

    ASSERT_EQ(false, it5.next(&pt)) << "Iterator should be finished";

    /*
     * Looping over ? ? o deletions in patch 1 starting from a non-existing start element that lies after the two elements
     */
    PositionedTripleIterator it6 = *patchTree->deletion_iterator_from(Triple("t", "t", "t", &dict), 1, Triple("", "", "o", &dict));
    ASSERT_EQ(false, it6.next(&pt)) << "Iterator should be finished";

    /*
     * Looping over all deletions in patch 4 starting from beginning
     */
    PositionedTripleIterator it7 = *patchTree->deletion_iterator_from(Triple("", "", "", &dict), 4, Triple("", "", "", &dict));

    ASSERT_EQ(true, it7.next(&pt)) << "Iterator has a no next value";
    ASSERT_EQ("g p o.", pt.triple.to_string(dict)) << "Element is incorrect";
    ASSERT_EQ(0, pt.position) << "Element is incorrect";

    ASSERT_EQ(true, it7.next(&pt)) << "Iterator has a no next value";
    ASSERT_EQ("h p o.", pt.triple.to_string(dict)) << "Element is incorrect";
    ASSERT_EQ(1, pt.position) << "Element is incorrect";

    ASSERT_EQ(true, it7.next(&pt)) << "Iterator has a no next value";
    ASSERT_EQ("h z o.", pt.triple.to_string(dict)) << "Element is incorrect";
    ASSERT_EQ(2, pt.position) << "Element is incorrect";

    ASSERT_EQ(true, it7.next(&pt)) << "Iterator has a no next value";
    ASSERT_EQ("s z o.", pt.triple.to_string(dict)) << "Element is incorrect";
    ASSERT_EQ(3, pt.position) << "Element is incorrect";

    ASSERT_EQ(false, it7.next(&pt)) << "Iterator should be finished";

    /*
     * Looping over h ? o deletions in patch 4 starting from g p o
     */
    PositionedTripleIterator it8 = *patchTree->deletion_iterator_from(Triple("g", "p", "o", &dict), 4, Triple("h", "", "o", &dict));

    ASSERT_EQ(true, it8.next(&pt)) << "Iterator has a no next value";
    ASSERT_EQ("h p o.", pt.triple.to_string(dict)) << "Element is incorrect";
    ASSERT_EQ(0, pt.position) << "Element is incorrect";

    ASSERT_EQ(true, it8.next(&pt)) << "Iterator has a no next value";
    ASSERT_EQ("h z o.", pt.triple.to_string(dict)) << "Element is incorrect";
    ASSERT_EQ(1, pt.position) << "Element is incorrect";

    ASSERT_EQ(false, it8.next(&pt)) << "Iterator should be finished";

    /*
     * Looping over h ? o deletions in patch 4 starting from h q o
     */
    PositionedTripleIterator it9 = *patchTree->deletion_iterator_from(Triple("h", "q", "o", &dict), 4, Triple("h", "", "o", &dict));

    ASSERT_EQ(true, it9.next(&pt)) << "Iterator has a no next value";
    ASSERT_EQ("h z o.", pt.triple.to_string(dict)) << "Element is incorrect";
    ASSERT_EQ(1, pt.position) << "Element is incorrect";

    ASSERT_EQ(false, it9.next(&pt)) << "Iterator should be finished";
}

TEST_F(PatchTreeTest, AdditionIterator) {
    Patch patch1(&dict);
    patch1.add(PatchElement(Triple("g", "p", "o", &dict), false));
    patch1.add(PatchElement(Triple("a", "p", "o", &dict), true));
    patchTree->append(patch1, 1);

    Patch patch2(&dict);
    patch2.add(PatchElement(Triple("s", "z", "o", &dict), false));
    patch2.add(PatchElement(Triple("s", "a", "o", &dict), true));
    patchTree->append(patch2, 1);

    Patch patch3(&dict);
    patch3.add(PatchElement(Triple("g", "p", "o", &dict), false));
    patch3.add(PatchElement(Triple("a", "p", "o", &dict), false));
    patch3.add(PatchElement(Triple("h", "z", "o", &dict), false));
    patch3.add(PatchElement(Triple("l", "a", "o", &dict), true));
    patchTree->append(patch3, 2);

    Patch patch4(&dict);
    patch4.add(PatchElement(Triple("h", "p", "o", &dict), false));
    patch4.add(PatchElement(Triple("s", "z", "o", &dict), false));
    patchTree->append(patch4, 4);

    Patch patch5(&dict);
    patch5.add(PatchElement(Triple("h", "p", "o", &dict), true));
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

    Triple pt;

    /*
     * Looping over all additions in patch 1 starting from beginning
     */
    ASSERT_EQ(2, patchTree->addition_count(1, Triple("", "", "", &dict))) << "Addition count is wrong";
    PatchTreeTripleIterator it1 = *patchTree->addition_iterator_from(0, 1, Triple("", "", "", &dict));

    ASSERT_EQ(true, it1.next(&pt)) << "Iterator has a no next value";
    ASSERT_EQ("a p o.", pt.to_string(dict)) << "Element is incorrect";

    ASSERT_EQ(true, it1.next(&pt)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", pt.to_string(dict)) << "Element is incorrect";

    ASSERT_EQ(false, it1.next(&pt)) << "Iterator should be finished";

    /*
     * Looping over s a o additions in patch 1 starting from beginning
     */
    ASSERT_EQ(1, patchTree->addition_count(1, Triple("s", "a", "o", &dict))) << "Addition count is wrong";
    PatchTreeTripleIterator it2 = *patchTree->addition_iterator_from(0, 1, Triple("s", "a", "o", &dict));

    ASSERT_EQ(true, it2.next(&pt)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", pt.to_string(dict)) << "Element is incorrect";

    ASSERT_EQ(false, it2.next(&pt)) << "Iterator should be finished";

    /*
     * Looping over all additions in patch 1 starting from 1
     */
    ASSERT_EQ(2, patchTree->addition_count(1, Triple("", "", "", &dict))) << "Addition count is wrong";
    PatchTreeTripleIterator it3 = *patchTree->addition_iterator_from(1, 1, Triple("", "", "", &dict));

    ASSERT_EQ(true, it3.next(&pt)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", pt.to_string(dict)) << "Element is incorrect";

    ASSERT_EQ(false, it3.next(&pt)) << "Iterator should be finished";

    /*
     * Looping over all additions in patch 2 starting from beginning
     */
    ASSERT_EQ(2, patchTree->addition_count(2, Triple("", "", "", &dict))) << "Addition count is wrong";
    PatchTreeTripleIterator it4 = *patchTree->addition_iterator_from(0, 2, Triple("", "", "", &dict));

    ASSERT_EQ(true, it4.next(&pt)) << "Iterator has a no next value";
    ASSERT_EQ("l a o.", pt.to_string(dict)) << "Element is incorrect";

    ASSERT_EQ(true, it4.next(&pt)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", pt.to_string(dict)) << "Element is incorrect";

    ASSERT_EQ(false, it4.next(&pt)) << "Iterator should be finished";

    /*
     * Looping over all additions in patch 5 starting from beginning
     */
    ASSERT_EQ(2, patchTree->addition_count(5, Triple("", "", "", &dict))) << "Addition count is wrong";
    PatchTreeTripleIterator it5 = *patchTree->addition_iterator_from(0, 5, Triple("", "", "", &dict));

    ASSERT_EQ(true, it5.next(&pt)) << "Iterator has a no next value";
    ASSERT_EQ("l a o.", pt.to_string(dict)) << "Element is incorrect";

    ASSERT_EQ(true, it5.next(&pt)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", pt.to_string(dict)) << "Element is incorrect";

    ASSERT_EQ(false, it5.next(&pt)) << "Iterator should be finished";
}

TEST_F(PatchTreeTest, AdditionIteratorOtherIndexes) {
    Patch patch1(&dict);
    patch1.add(PatchElement(Triple("g", "p", "o", &dict), false));
    patch1.add(PatchElement(Triple("a", "p", "o", &dict), true));
    patchTree->append(patch1, 1);

    Patch patch2(&dict);
    patch2.add(PatchElement(Triple("s", "z", "o", &dict), false));
    patch2.add(PatchElement(Triple("s", "a", "o", &dict), true));
    patchTree->append(patch2, 1);

    Patch patch3(&dict);
    patch3.add(PatchElement(Triple("g", "p", "o", &dict), false));
    patch3.add(PatchElement(Triple("a", "p", "o", &dict), false));
    patch3.add(PatchElement(Triple("h", "z", "o", &dict), false));
    patch3.add(PatchElement(Triple("l", "a", "o", &dict), true));
    patchTree->append(patch3, 2);

    Patch patch4(&dict);
    patch4.add(PatchElement(Triple("h", "p", "o", &dict), false));
    patch4.add(PatchElement(Triple("s", "z", "o", &dict), false));
    patchTree->append(patch4, 4);

    Patch patch5(&dict);
    patch5.add(PatchElement(Triple("h", "p", "o", &dict), true));
    patchTree->append(patch5, 5);

    Patch patch6(&dict);
    patch6.add(PatchElement(Triple("a", "p", "o", &dict), true));
    patchTree->append(patch6, 6);

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

    // Expected patch 6:
    // a p o +
    // g p o -
    // h p o -/+ (=> ignored)
    // h z o -
    // l a o +
    // s a o +
    // s z o -

    Triple pt;

    /*
     * Looping over ? ? ? additions in patch 5
     */
    ASSERT_EQ(2, patchTree->addition_count(5, Triple("", "", "", &dict))) << "Addition count is wrong";
    PatchTreeTripleIterator it1 = *patchTree->addition_iterator_from(0, 5, Triple("", "", "", &dict));

    ASSERT_EQ(true, it1.next(&pt)) << "Iterator has a no next value";
    ASSERT_EQ("l a o.", pt.to_string(dict)) << "Element is incorrect";

    ASSERT_EQ(true, it1.next(&pt)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", pt.to_string(dict)) << "Element is incorrect";

    ASSERT_EQ(false, it1.next(&pt)) << "Iterator should be finished";

    /*
     * Looping over ? ? o additions in patch 5
     */
    ASSERT_EQ(2, patchTree->addition_count(5, Triple("", "", "o", &dict))) << "Addition count is wrong";
    PatchTreeTripleIterator it2 = *patchTree->addition_iterator_from(0, 5, Triple("", "", "o", &dict));

    ASSERT_EQ(true, it2.next(&pt)) << "Iterator has a no next value";
    ASSERT_EQ("l a o.", pt.to_string(dict)) << "Element is incorrect";

    ASSERT_EQ(true, it2.next(&pt)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", pt.to_string(dict)) << "Element is incorrect";

    ASSERT_EQ(false, it2.next(&pt)) << "Iterator should be finished";

    /*
     * Looping over ? a ? additions in patch 5
     */
    ASSERT_EQ(2, patchTree->addition_count(5, Triple("", "a", "", &dict))) << "Addition count is wrong";
    PatchTreeTripleIterator it3 = *patchTree->addition_iterator_from(0, 5, Triple("", "a", "", &dict));

    ASSERT_EQ(true, it3.next(&pt)) << "Iterator has a no next value";
    ASSERT_EQ("l a o.", pt.to_string(dict)) << "Element is incorrect";

    ASSERT_EQ(true, it3.next(&pt)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", pt.to_string(dict)) << "Element is incorrect";

    ASSERT_EQ(false, it3.next(&pt)) << "Iterator should be finished";

    /*
     * Looping over ? a o additions in patch 5
     */
    ASSERT_EQ(2, patchTree->addition_count(5, Triple("", "a", "o", &dict))) << "Addition count is wrong";
    PatchTreeTripleIterator it4 = *patchTree->addition_iterator_from(0, 5, Triple("", "a", "o", &dict));

    ASSERT_EQ(true, it4.next(&pt)) << "Iterator has a no next value";
    ASSERT_EQ("l a o.", pt.to_string(dict)) << "Element is incorrect";

    ASSERT_EQ(true, it4.next(&pt)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", pt.to_string(dict)) << "Element is incorrect";

    ASSERT_EQ(false, it4.next(&pt)) << "Iterator should be finished";

    /*
     * Looping over s ? o additions in patch 5
     */
    ASSERT_EQ(1, patchTree->addition_count(5, Triple("s", "", "o", &dict))) << "Addition count is wrong";
    PatchTreeTripleIterator it5 = *patchTree->addition_iterator_from(0, 5, Triple("s", "", "o", &dict));

    ASSERT_EQ(true, it5.next(&pt)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", pt.to_string(dict)) << "Element is incorrect";

    ASSERT_EQ(false, it5.next(&pt)) << "Iterator should be finished";

    /*
     * Looping over s a ? additions in patch 5
     */
    ASSERT_EQ(1, patchTree->addition_count(5, Triple("s", "a", "", &dict))) << "Addition count is wrong";
    PatchTreeTripleIterator it6 = *patchTree->addition_iterator_from(0, 5, Triple("s", "a", "", &dict));

    ASSERT_EQ(true, it6.next(&pt)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", pt.to_string(dict)) << "Element is incorrect";

    ASSERT_EQ(false, it6.next(&pt)) << "Iterator should be finished";

    /*
     * Looping over s a o additions in patch 5
     */
    ASSERT_EQ(1, patchTree->addition_count(5, Triple("s", "a", "o", &dict))) << "Addition count is wrong";
    PatchTreeTripleIterator it7 = *patchTree->addition_iterator_from(0, 5, Triple("s", "a", "o", &dict));

    ASSERT_EQ(true, it7.next(&pt)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", pt.to_string(dict)) << "Element is incorrect";

    ASSERT_EQ(false, it7.next(&pt)) << "Iterator should be finished";

    /*
     * Looping over s ? o additions in patch 5
     */
    ASSERT_EQ(1, patchTree->addition_count(5, Triple("s", "", "", &dict))) << "Addition count is wrong";
    PatchTreeTripleIterator it7_2 = *patchTree->addition_iterator_from(0, 5, Triple("s", "", "", &dict));

    ASSERT_EQ(true, it7_2.next(&pt)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", pt.to_string(dict)) << "Element is incorrect";

    ASSERT_EQ(false, it7_2.next(&pt)) << "Iterator should be finished";

    /*
     * Looping over ? ? ? additions in patch 6
     */
    ASSERT_EQ(3, patchTree->addition_count(6, Triple("", "", "", &dict))) << "Addition count is wrong";
    PatchTreeTripleIterator it8 = *patchTree->addition_iterator_from(0, 6, Triple("", "", "", &dict));

    ASSERT_EQ(true, it8.next(&pt)) << "Iterator has a no next value";
    ASSERT_EQ("a p o.", pt.to_string(dict)) << "Element is incorrect";

    ASSERT_EQ(true, it8.next(&pt)) << "Iterator has a no next value";
    ASSERT_EQ("l a o.", pt.to_string(dict)) << "Element is incorrect";

    ASSERT_EQ(true, it8.next(&pt)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", pt.to_string(dict)) << "Element is incorrect";

    ASSERT_EQ(false, it8.next(&pt)) << "Iterator should be finished";

    /*
     * Looping over ? ? o additions in patch 6
     */
    ASSERT_EQ(3, patchTree->addition_count(6, Triple("", "", "o", &dict))) << "Addition count is wrong";
    PatchTreeTripleIterator it9 = *patchTree->addition_iterator_from(0, 6, Triple("", "", "o", &dict));

    ASSERT_EQ(true, it9.next(&pt)) << "Iterator has a no next value";
    ASSERT_EQ("a p o.", pt.to_string(dict)) << "Element is incorrect";

    ASSERT_EQ(true, it9.next(&pt)) << "Iterator has a no next value";
    ASSERT_EQ("l a o.", pt.to_string(dict)) << "Element is incorrect";

    ASSERT_EQ(true, it9.next(&pt)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", pt.to_string(dict)) << "Element is incorrect";

    ASSERT_EQ(false, it9.next(&pt)) << "Iterator should be finished";

    /*
     * Looping over ? p ? additions in patch 6
     */
    ASSERT_EQ(1, patchTree->addition_count(6, Triple("", "p", "", &dict))) << "Addition count is wrong";
    PatchTreeTripleIterator it10 = *patchTree->addition_iterator_from(0, 6, Triple("", "p", "", &dict));

    ASSERT_EQ(true, it10.next(&pt)) << "Iterator has a no next value";
    ASSERT_EQ("a p o.", pt.to_string(dict)) << "Element is incorrect";

    ASSERT_EQ(false, it10.next(&pt)) << "Iterator should be finished";
}

TEST_F(PatchTreeTest, Metadata) {
    ASSERT_EQ(0, patchTree->get_min_patch_id()) << "Min patch id is incorrect";
    ASSERT_EQ(0, patchTree->get_max_patch_id()) << "Max patch id is incorrect";

    Patch patch1(&dict);
    patch1.add(PatchElement(Triple("s1", "p1", "o1", &dict), true));
    patchTree->append_unsafe(patch1, 0);

    ASSERT_EQ(0, patchTree->get_min_patch_id()) << "Min patch id is incorrect";
    ASSERT_EQ(0, patchTree->get_max_patch_id()) << "Max patch id is incorrect";

    Patch patch2(&dict);
    patch2.add(PatchElement(Triple("s1", "p1", "o1", &dict), true));
    patchTree->append_unsafe(patch2, 1);

    ASSERT_EQ(0, patchTree->get_min_patch_id()) << "Min patch id is incorrect";
    ASSERT_EQ(1, patchTree->get_max_patch_id()) << "Max patch id is incorrect";

    delete patchTree;
    patchTree = new PatchTree(0, &dict);

    ASSERT_EQ(0, patchTree->get_min_patch_id()) << "Min patch id is incorrect";
    ASSERT_EQ(1, patchTree->get_max_patch_id()) << "Max patch id is incorrect";
}
