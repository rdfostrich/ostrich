#include <gtest/gtest.h>

#include "../../../main/cpp/controller/controller.h"
#include "../../../main/cpp/snapshot/vector_triple_iterator.h"
#include "../../../main/cpp/dictionary/dictionary_manager.h"

#define BASEURI "<http://example.org>"

// The fixture for testing class Controller.
class ControllerTest : public ::testing::Test {
protected:
    Controller controller;

    ControllerTest() : controller() {}

    virtual void SetUp() {

    }

    virtual void TearDown() {
        // Delete patch files
        std::map<int, PatchTree*> patches = controller.get_patch_tree_manager()->get_patch_trees();
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

        // Delete snapshot files
        std::map<int, HDT*> snapshots = controller.get_snapshot_manager()->get_snapshots();
        std::map<int, HDT*>::iterator itS = snapshots.begin();
        while(itS != snapshots.end()) {
            int id = itS->first;
            std::remove(SNAPSHOT_FILENAME_BASE(id).c_str());
            std::remove((SNAPSHOT_FILENAME_BASE(id) + ".index").c_str());
            itS++;
        }
    }
};

TEST_F(ControllerTest, GetEdge) {
    Triple t;

    // Build a snapshot
    std::vector<TripleString> triples;
    triples.push_back(TripleString("<a>", "<a>", "<a>"));
    triples.push_back(TripleString("<a>", "<a>", "<b>"));
    triples.push_back(TripleString("<a>", "<a>", "<c>"));
    VectorTripleIterator* it = new VectorTripleIterator(triples);
    controller.get_snapshot_manager()->create_snapshot(0, it, BASEURI);
    PatchTreeManager* patchTreeManager = controller.get_patch_tree_manager();

    DictionaryManager *dict = controller.get_snapshot_manager()->get_dictionary_manager(0);

    // Request version 1 (after snapshot before a patch id added)
    TripleIterator* it1 = controller.get(Triple("", "", "", dict), 0, 1);

    ASSERT_EQ(true, it1->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <a>.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it1->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <b>.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it1->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <c>.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(false, it1->next(&t)) << "Iterator should be finished";

    // Apply a simple patch
    Patch patch1(dict);
    patch1.add(PatchElement(Triple("<a>", "<a>", "<b>", dict), false));
    patchTreeManager->append(patch1, 1, dict);

    // Request version -1 (before first snapshot)
    TripleIterator* it0 = controller.get(Triple("", "", "", dict), 0, -1);
    ASSERT_EQ(false, it0->next(&t)) << "Iterator should be empty";

    // Request version 2 (after last patch)
    TripleIterator* it2 = controller.get(Triple("", "", "", dict), 0, 2);

    ASSERT_EQ(true, it2->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <a>.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it2->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <c>.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(false, it2->next(&t)) << "Iterator should be finished";
}

TEST_F(ControllerTest, GetSimple) {
    // Build a snapshot
    std::vector<TripleString> triples;
    triples.push_back(TripleString("<a>", "<a>", "<a>"));
    triples.push_back(TripleString("<a>", "<a>", "<b>"));
    triples.push_back(TripleString("<a>", "<a>", "<c>"));
    VectorTripleIterator* it = new VectorTripleIterator(triples);
    controller.get_snapshot_manager()->create_snapshot(0, it, BASEURI);
    PatchTreeManager* patchTreeManager = controller.get_patch_tree_manager();
    DictionaryManager *dict = controller.get_snapshot_manager()->get_dictionary_manager(0);

    // Apply a simple patch
    Patch patch1(dict);
    patch1.add(PatchElement(Triple("<a>", "<a>", "<b>", dict), false));
    patchTreeManager->append(patch1, 1, dict);

    // Expected version 0:
    // <a> <a> <a>
    // <a> <a> <b>
    // <a> <a> <c>

    // Expected version 1:
    // <a> <a> <a>
    // <a> <a> <c>

    Triple t;

    // Request version 0 (snapshot)
    TripleIterator* it0 = controller.get(Triple("", "", "", dict), 0, 0);

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <a>.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <b>.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <c>.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(false, it0->next(&t)) << "Iterator should be finished";

    // Request version 1 (patch)
    TripleIterator* it1 = controller.get(Triple("", "", "", dict), 0, 1);

    ASSERT_EQ(true, it1->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <a>.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it1->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <c>.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(false, it1->next(&t)) << "Iterator should be finished";

    // Request version 1 (patch) at offset 1
    TripleIterator* it2 = controller.get(Triple("", "", "", dict), 1, 1);

    ASSERT_EQ(true, it2->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <c>.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(false, it2->next(&t)) << "Iterator should be finished";

    // Request version 0 (snapshot) for ? ? <c>
    TripleIterator* it3 = controller.get(Triple("", "", "<c>", dict), 0, 0);

    ASSERT_EQ(true, it3->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <c>.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(false, it3->next(&t)) << "Iterator should be finished";

    // Request version 1 (patch) for ? ? <c>
    TripleIterator* it4 = controller.get(Triple("", "", "<c>", dict), 0, 1);

    ASSERT_EQ(true, it4->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <c>.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(false, it4->next(&t)) << "Iterator should be finished";
}

TEST_F(ControllerTest, GetComplex1) {
    // Build a snapshot
    std::vector<TripleString> triples;
    triples.push_back(TripleString("g", "p", "o"));
    triples.push_back(TripleString("s", "z", "o"));
    triples.push_back(TripleString("h", "z", "o"));
    triples.push_back(TripleString("h", "p", "o"));
    VectorTripleIterator* it = new VectorTripleIterator(triples);
    controller.get_snapshot_manager()->create_snapshot(0, it, BASEURI);
    PatchTreeManager* patchTreeManager = controller.get_patch_tree_manager();
    DictionaryManager *dict = controller.get_snapshot_manager()->get_dictionary_manager(0);

    Patch patch1(dict);
    patch1.add(PatchElement(Triple("g", "p", "o", dict), false));
    patch1.add(PatchElement(Triple("a", "p", "o", dict), true));
    patchTreeManager->append(patch1, 1, dict);

    Patch patch2(dict);
    patch2.add(PatchElement(Triple("s", "z", "o", dict), false));
    patch2.add(PatchElement(Triple("s", "a", "o", dict), true));
    patchTreeManager->append(patch2, 1, dict);

    Patch patch3(dict);
    patch3.add(PatchElement(Triple("g", "p", "o", dict), false));
    patch3.add(PatchElement(Triple("a", "p", "o", dict), false));
    patch3.add(PatchElement(Triple("h", "z", "o", dict), false));
    patch3.add(PatchElement(Triple("l", "a", "o", dict), true));
    patchTreeManager->append(patch3, 2, dict);

    Patch patch4(dict);
    patch4.add(PatchElement(Triple("h", "p", "o", dict), false));
    patch4.add(PatchElement(Triple("s", "z", "o", dict), false));
    patchTreeManager->append(patch4, 4, dict);

    Patch patch5(dict);
    patch5.add(PatchElement(Triple("h", "p", "o", dict), true));
    patchTreeManager->append(patch5, 5, dict);

    Patch patch6(dict);
    patch6.add(PatchElement(Triple("a", "p", "o", dict), true));
    patchTreeManager->append(patch6, 6, dict);

    // Expected version 0:
    // g p o
    // h p o
    // h z o
    // s z o

    // Expected version 1:
    // h p o
    // h z o
    // a p o (+)
    // s a o (+)

    // Expected version 2:
    // h p o
    // l a o (+)
    // s a o (+)

    Triple t;

    // Request version 0 (snapshot)
    TripleIterator* it0 = controller.get(Triple("", "", "", dict), 0, 0);

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("g p o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("h p o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("h z o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("s z o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(false, it0->next(&t)) << "Iterator should be finished";

    // Request version 1 (patch)
    TripleIterator* it1 = controller.get(Triple("", "", "", dict), 0, 1);

    ASSERT_EQ(true, it1->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("h p o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it1->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("h z o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it1->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("a p o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it1->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(false, it1->next(&t)) << "Iterator should be finished";

    // Request version 1 (patch), offset 1
    TripleIterator* it2 = controller.get(Triple("", "", "", dict), 1, 1);

    ASSERT_EQ(true, it2->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("h z o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it2->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("a p o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it2->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(false, it2->next(&t)) << "Iterator should be finished";

    // Request version 1 (patch), offset 2
    TripleIterator* it3 = controller.get(Triple("", "", "", dict), 2, 1);

    ASSERT_EQ(true, it3->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("a p o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it3->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(false, it3->next(&t)) << "Iterator should be finished";

    // Request version 1 (patch), offset 3
    TripleIterator* it4 = controller.get(Triple("", "", "", dict), 3, 1);

    ASSERT_EQ(true, it4->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(false, it4->next(&t)) << "Iterator should be finished";

    // Request version 1 (patch), offset 4
    TripleIterator* it5 = controller.get(Triple("", "", "", dict), 4, 1);

    ASSERT_EQ(false, it5->next(&t)) << "Iterator should be finished";

    // Request version 2 (patch)
    TripleIterator* it6 = controller.get(Triple("", "", "", dict), 0, 2);

    ASSERT_EQ(true, it6->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("h p o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it6->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("l a o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it6->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(false, it6->next(&t)) << "Iterator should be finished";

    // Request version 2 (patch), offset 1
    TripleIterator* it7 = controller.get(Triple("", "", "", dict), 1, 2);

    ASSERT_EQ(true, it7->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("l a o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it7->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(false, it7->next(&t)) << "Iterator should be finished";

    // Request version 2 (patch), offset 2
    TripleIterator* it8 = controller.get(Triple("", "", "", dict), 2, 2);

    ASSERT_EQ(true, it8->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(false, it8->next(&t)) << "Iterator should be finished";

    // Request version 2 (patch), offset 3
    TripleIterator* it9 = controller.get(Triple("", "", "", dict), 3, 2);

    ASSERT_EQ(false, it9->next(&t)) << "Iterator should be finished";
}

TEST_F(ControllerTest, GetComplex2) {
    // Build a snapshot
    std::vector<TripleString> triples;
    triples.push_back(TripleString("g", "p", "o"));
    triples.push_back(TripleString("s", "z", "o"));
    triples.push_back(TripleString("h", "z", "o"));
    triples.push_back(TripleString("h", "p", "o"));
    VectorTripleIterator* it = new VectorTripleIterator(triples);
    controller.get_snapshot_manager()->create_snapshot(0, it, BASEURI);
    PatchTreeManager* patchTreeManager = controller.get_patch_tree_manager();
    DictionaryManager *dict = controller.get_snapshot_manager()->get_dictionary_manager(0);

    Patch patch1(dict);
    patch1.add(PatchElement(Triple("g", "p", "o", dict), false));
    patch1.add(PatchElement(Triple("a", "p", "o", dict), true));
    patchTreeManager->append(patch1, 1, dict);

    Patch patch2(dict);
    patch2.add(PatchElement(Triple("s", "z", "o", dict), false));
    patch2.add(PatchElement(Triple("s", "a", "o", dict), true));
    patchTreeManager->append(patch2, 1, dict);

    Patch patch3(dict);
    patch3.add(PatchElement(Triple("g", "p", "o", dict), false));
    patch3.add(PatchElement(Triple("a", "p", "o", dict), false));
    patch3.add(PatchElement(Triple("h", "z", "o", dict), false));
    patch3.add(PatchElement(Triple("l", "a", "o", dict), true));
    patchTreeManager->append(patch3, 2, dict);

    Patch patch4(dict);
    patch4.add(PatchElement(Triple("h", "p", "o", dict), false));
    patch4.add(PatchElement(Triple("s", "z", "o", dict), false));
    patchTreeManager->append(patch4, 4, dict);

    Patch patch5(dict);
    patch5.add(PatchElement(Triple("h", "p", "o", dict), true));
    patchTreeManager->append(patch5, 5, dict);

    Patch patch6(dict);
    patch6.add(PatchElement(Triple("a", "p", "o", dict), true));
    patchTreeManager->append(patch6, 6, dict);

    // Expected version 0:
    // g p o
    // h p o
    // h z o
    // s z o

    // Expected version 1:
    // h p o
    // h z o
    // a p o (+)
    // s a o (+)

    // Expected version 2:
    // h p o
    // l a o (+)
    // s a o (+)

    Triple t;

    // Request version 0 (snapshot)
    TripleIterator* it0 = controller.get(Triple("", "", "o", dict), 0, 0);

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("g p o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("h p o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("h z o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("s z o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(false, it0->next(&t)) << "Iterator should be finished";

    // Request version 1 (patch)
    TripleIterator* it1 = controller.get(Triple("", "", "o", dict), 0, 1);

    ASSERT_EQ(true, it1->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("h p o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it1->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("h z o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it1->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("a p o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it1->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(false, it1->next(&t)) << "Iterator should be finished";

    // Request version 1 (patch), offset 1
    TripleIterator* it2 = controller.get(Triple("", "", "o", dict), 1, 1);

    ASSERT_EQ(true, it2->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("h z o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it2->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("a p o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it2->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(false, it2->next(&t)) << "Iterator should be finished";

    // Request version 1 (patch), offset 2
    TripleIterator* it3 = controller.get(Triple("", "", "o", dict), 2, 1);

    ASSERT_EQ(true, it3->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("a p o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it3->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(false, it3->next(&t)) << "Iterator should be finished";

    // Request version 1 (patch), offset 3
    TripleIterator* it4 = controller.get(Triple("", "", "o", dict), 3, 1);

    ASSERT_EQ(true, it4->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(false, it4->next(&t)) << "Iterator should be finished";

    // Request version 1 (patch), offset 4
    TripleIterator* it5 = controller.get(Triple("", "", "o", dict), 4, 1);

    ASSERT_EQ(false, it5->next(&t)) << "Iterator should be finished";

    // Request version 2 (patch)
    TripleIterator* it6 = controller.get(Triple("", "", "o", dict), 0, 2);

    ASSERT_EQ(true, it6->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("h p o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it6->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("l a o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it6->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(false, it6->next(&t)) << "Iterator should be finished";

    // Request version 2 (patch), offset 1
    TripleIterator* it7 = controller.get(Triple("", "", "o", dict), 1, 2);

    ASSERT_EQ(true, it7->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("l a o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it7->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(false, it7->next(&t)) << "Iterator should be finished";

    // Request version 2 (patch), offset 2
    TripleIterator* it8 = controller.get(Triple("", "", "o", dict), 2, 2);

    ASSERT_EQ(true, it8->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(false, it8->next(&t)) << "Iterator should be finished";

    // Request version 2 (patch), offset 3
    TripleIterator* it9 = controller.get(Triple("", "", "o", dict), 3, 2);

    ASSERT_EQ(false, it9->next(&t)) << "Iterator should be finished";
}

TEST_F(ControllerTest, GetComplex3) {
    // Build a snapshot
    std::vector<TripleString> triples;
    triples.push_back(TripleString("g", "p", "o"));
    triples.push_back(TripleString("s", "z", "o"));
    triples.push_back(TripleString("h", "z", "o"));
    triples.push_back(TripleString("h", "p", "o"));
    VectorTripleIterator* it = new VectorTripleIterator(triples);
    controller.get_snapshot_manager()->create_snapshot(0, it, BASEURI);
    PatchTreeManager* patchTreeManager = controller.get_patch_tree_manager();
    DictionaryManager *dict = controller.get_snapshot_manager()->get_dictionary_manager(0);

    Patch patch1(dict);
    patch1.add(PatchElement(Triple("g", "p", "o", dict), false));
    patch1.add(PatchElement(Triple("a", "p", "o", dict), true));
    patchTreeManager->append(patch1, 1, dict);

    Patch patch2(dict);
    patch2.add(PatchElement(Triple("s", "z", "o", dict), false));
    patch2.add(PatchElement(Triple("s", "a", "o", dict), true));
    patchTreeManager->append(patch2, 1, dict);

    Patch patch3(dict);
    patch3.add(PatchElement(Triple("g", "p", "o", dict), false));
    patch3.add(PatchElement(Triple("a", "p", "o", dict), false));
    patch3.add(PatchElement(Triple("h", "z", "o", dict), false));
    patch3.add(PatchElement(Triple("l", "a", "o", dict), true));
    patchTreeManager->append(patch3, 2, dict);

    Patch patch4(dict);
    patch4.add(PatchElement(Triple("h", "p", "o", dict), false));
    patch4.add(PatchElement(Triple("s", "z", "o", dict), false));
    patchTreeManager->append(patch4, 4, dict);

    Patch patch5(dict);
    patch5.add(PatchElement(Triple("h", "p", "o", dict), true));
    patchTreeManager->append(patch5, 5, dict);

    Patch patch6(dict);
    patch6.add(PatchElement(Triple("a", "p", "o", dict), true));
    patchTreeManager->append(patch6, 6, dict);

    // --- filtered by s ? ?

    // Expected version 0:
    // s z o

    // Expected version 1:
    // s a o (+)

    // Expected version 2:
    // s a o (+)

    Triple t;

    // Request version 0 (snapshot)
    TripleIterator* it0 = controller.get(Triple("s", "", "", dict), 0, 0);

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("s z o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(false, it0->next(&t)) << "Iterator should be finished";

    // Request version 1 (patch)
    TripleIterator* it1 = controller.get(Triple("s", "", "", dict), 0, 1);

    ASSERT_EQ(true, it1->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(false, it1->next(&t)) << "Iterator should be finished";

    // Request version 1 (patch), offset 1
    TripleIterator* it2 = controller.get(Triple("s", "", "", dict), 1, 1);

    ASSERT_EQ(false, it2->next(&t)) << "Iterator should be finished";

    // Request version 2 (patch)
    TripleIterator* it6 = controller.get(Triple("s", "", "", dict), 0, 2);

    ASSERT_EQ(true, it6->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(false, it6->next(&t)) << "Iterator should be finished";

    // Request version 2 (patch), offset 1
    TripleIterator* it7 = controller.get(Triple("s", "", "", dict), 1, 2);

    ASSERT_EQ(false, it7->next(&t)) << "Iterator should be finished";
}

TEST_F(ControllerTest, EdgeCase1) {
    /*
     * This tests the special case where there are extra deletions that
     * need to be taken into account when the first deletion count is applied
     * as offset to the snapshot iterator.
     *
     * In this case, for offset = 3 we will find in the snapshot element "3".
     * The number of deletions before this element is 3, so the new snapshot offset becomes 6 (= 3 + 3).
     * The first element in this new snapshot iterator is "6", which is wrong because we expect "8".
     * This is because deletion elements "4" and "5" weren't taken into account when applying this new offset.
     * This is why we have to _loop_ when applying offsets until we apply one that contains no new deletions relative
     * to the previous offset.
     */

    // Build a snapshot
    std::vector<TripleString> triples;
    triples.push_back(TripleString("0", "0", "0"));
    triples.push_back(TripleString("1", "1", "1"));
    triples.push_back(TripleString("2", "2", "2"));
    triples.push_back(TripleString("3", "3", "3"));
    triples.push_back(TripleString("4", "4", "4"));
    triples.push_back(TripleString("5", "5", "5"));
    triples.push_back(TripleString("6", "6", "6"));
    triples.push_back(TripleString("7", "7", "7"));
    triples.push_back(TripleString("8", "8", "8"));
    triples.push_back(TripleString("9", "9", "9"));
    VectorTripleIterator *it = new VectorTripleIterator(triples);
    controller.get_snapshot_manager()->create_snapshot(0, it, BASEURI);
    PatchTreeManager* patchTreeManager = controller.get_patch_tree_manager();
    DictionaryManager *dict = controller.get_snapshot_manager()->get_dictionary_manager(0);

    Patch patch1(dict);
    patch1.add(PatchElement(Triple("0", "0", "0", dict), false));
    patch1.add(PatchElement(Triple("1", "1", "1", dict), false));
    patch1.add(PatchElement(Triple("2", "2", "2", dict), false));
    // No 3!
    patch1.add(PatchElement(Triple("4", "4", "4", dict), false));
    patch1.add(PatchElement(Triple("5", "5", "5", dict), false));
    patchTreeManager->append(patch1, 1, dict);

    Triple t;

    // Request version 1, offset 0
    TripleIterator* it0 = controller.get(Triple("", "", "", dict), 0, 1);

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("3 3 3.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("6 6 6.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("7 7 7.", t.to_string(*dict)) << "Element is incorrect";

    // Request version 1, offset 3 (The actual edge case!)
    TripleIterator* it1 = controller.get(Triple("", "", "", dict), 3, 1);

    ASSERT_EQ(true, it1->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("8 8 8.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it1->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("9 9 9.", t.to_string(*dict)) << "Element is incorrect";
}
