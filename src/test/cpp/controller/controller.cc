#include <gtest/gtest.h>

#include "../../../main/cpp/controller/controller.h"
#include "../../../main/cpp/snapshot/vector_triple_iterator.h"

#define BASEURI "<http://example.org>"

// The fixture for testing class Controller.
class ControllerTest : public ::testing::Test {
protected:
    Controller controller;

    ControllerTest() : controller() {}

    virtual void SetUp() {
        controller = Controller();
    }

    virtual void TearDown() {
        // Delete patch files
        std::map<int, PatchTree*> patches = controller.get_patch_trees();
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

TEST_F(ControllerTest, ConstructPatch) {
    ASSERT_EQ((PatchTree*) NULL, controller.get_patch_tree(0)) << "Patch tree with id 0 should not be present.";

    controller.construct_next_patch_tree(0);

    ASSERT_NE((PatchTree*) NULL, controller.get_patch_tree(0)) << "Patch tree with id 0 should not be null.";
}

TEST_F(ControllerTest, DetectPatchTrees) {
    std::map<int, PatchTree*> found_patches1 = controller.detect_patch_trees();
    ASSERT_EQ(true, found_patches1.empty()) << "No patch trees should be detected";

    controller.construct_next_patch_tree(0);
    std::map<int, PatchTree*> found_patches2 = controller.detect_patch_trees();
    ASSERT_EQ(false, found_patches2.empty()) << "One patch tree should be detected";
    ASSERT_EQ(1, found_patches2.size()) << "One patch tree should be detected";

    controller.construct_next_patch_tree(1);
    std::map<int, PatchTree*> found_patches3 = controller.detect_patch_trees();
    ASSERT_EQ(false, found_patches3.empty()) << "Two patch trees should be detected";
    ASSERT_EQ(2, found_patches3.size()) << "Two patch trees should be detected";
}

TEST_F(ControllerTest, GetPatchTreeId) {
    ASSERT_EQ(-1, controller.get_patch_tree_id(-1));
    ASSERT_EQ(-1, controller.get_patch_tree_id(0));
    ASSERT_EQ(-1, controller.get_patch_tree_id(1));

    controller.construct_next_patch_tree(0);
    ASSERT_EQ(-1, controller.get_patch_tree_id(-1));
    ASSERT_EQ(0, controller.get_patch_tree_id(0));
    ASSERT_EQ(0, controller.get_patch_tree_id(1));

    controller.construct_next_patch_tree(10);
    ASSERT_EQ(-1, controller.get_patch_tree_id(-1));
    ASSERT_EQ(0, controller.get_patch_tree_id(0));
    ASSERT_EQ(0, controller.get_patch_tree_id(1));
    ASSERT_EQ(0, controller.get_patch_tree_id(9));
    ASSERT_EQ(10, controller.get_patch_tree_id(10));
    ASSERT_EQ(10, controller.get_patch_tree_id(11));
    ASSERT_EQ(10, controller.get_patch_tree_id(100));

    controller.construct_next_patch_tree(100);
    ASSERT_EQ(-1, controller.get_patch_tree_id(-1));
    ASSERT_EQ(0, controller.get_patch_tree_id(0));
    ASSERT_EQ(0, controller.get_patch_tree_id(1));
    ASSERT_EQ(0, controller.get_patch_tree_id(9));
    ASSERT_EQ(10, controller.get_patch_tree_id(10));
    ASSERT_EQ(10, controller.get_patch_tree_id(11));
    ASSERT_EQ(10, controller.get_patch_tree_id(99));
    ASSERT_EQ(100, controller.get_patch_tree_id(100));
    ASSERT_EQ(100, controller.get_patch_tree_id(101));
}

TEST_F(ControllerTest, AppendPatch) {
    Patch patch1;
    patch1.add(PatchElement(Triple("s1", "p1", "o1"), true));
    patch1.add(PatchElement(Triple("s2", "p2", "o2"), false));
    patch1.add(PatchElement(Triple("s3", "p3", "o3"), false));
    patch1.add(PatchElement(Triple("s4", "p4", "o4"), true));

    Patch patch2;
    patch2.add(PatchElement(Triple("s1", "p1", "o1"), true));

    Patch patch3;
    patch3.add(PatchElement(Triple("a", "b", "c"), true));

    ASSERT_EQ(true, controller.append(patch1, 0));
    ASSERT_EQ(false, controller.append(patch1, 0)) << "Append shouldn't allow for double appends";
    ASSERT_EQ(false, controller.append(patch2, 0)) << "Append shouldn't allow for double appends, not even partial";
    ASSERT_EQ(true, controller.append(patch3, 1));
}

TEST_F(ControllerTest, GetPatch) {
    Patch patch1;
    patch1.add(PatchElement(Triple("s1", "p1", "o1"), true));
    patch1.add(PatchElement(Triple("s2", "p2", "o2"), false));
    patch1.add(PatchElement(Triple("s3", "p3", "o3"), false));
    patch1.add(PatchElement(Triple("s4", "p4", "o4"), true));

    Patch patch2;
    patch2.add(PatchElement(Triple("a", "b", "c"), true));

    Patch patch3;
    patch3.add(PatchElement(Triple("s4", "p4", "o4"), false));

    controller.append(patch1, 0);
    controller.append(patch2, 1);
    controller.append(patch3, 2);

    ASSERT_EQ("s1 p1 o1. (+)\ns2 p2 o2. (-)\ns3 p3 o3. (-)\ns4 p4 o4. (+)\n", controller.get_patch(0).to_string());
    ASSERT_EQ("a b c. (+)\ns1 p1 o1. (+)\ns2 p2 o2. (-)\ns3 p3 o3. (-)\ns4 p4 o4. (+)\n", controller.get_patch(1).to_string());
    ASSERT_EQ("a b c. (+)\ns1 p1 o1. (+)\ns2 p2 o2. (-)\ns3 p3 o3. (-)\n", controller.get_patch(2).to_string());
}

TEST_F(ControllerTest, GetSimple) {
    // Build a snapshot
    std::vector<TripleString> triples;
    triples.push_back(TripleString("<a>", "<a>", "<a>"));
    triples.push_back(TripleString("<a>", "<a>", "<b>"));
    triples.push_back(TripleString("<a>", "<a>", "<c>"));
    VectorTripleIterator* it = new VectorTripleIterator(triples);
    controller.get_snapshot_manager()->create_snapshot(0, it, BASEURI);

    // Apply a simple patch
    Patch patch1;
    patch1.add(PatchElement(Triple("<a>", "<a>", "<b>"), false));
    controller.append(patch1, 1);

    // Expected version 0:
    // <a> <a> <a>
    // <a> <a> <b>
    // <a> <a> <c>

    // Expected version 1:
    // <a> <a> <a>
    // <a> <a> <c>

    Triple t;

    // Request version 0 (snapshot)
    TripleIterator* it0 = controller.get(Triple("", "", ""), 0, 0);

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <a>.", t.to_string()) << "Element is incorrect";

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <b>.", t.to_string()) << "Element is incorrect";

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <c>.", t.to_string()) << "Element is incorrect";

    ASSERT_EQ(false, it0->next(&t)) << "Iterator should be finished";
    
    // Request version 1 (patch)
    TripleIterator* it1 = controller.get(Triple("", "", ""), 0, 1);

    ASSERT_EQ(true, it1->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <a>.", t.to_string()) << "Element is incorrect";

    ASSERT_EQ(true, it1->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <c>.", t.to_string()) << "Element is incorrect";

    ASSERT_EQ(false, it1->next(&t)) << "Iterator should be finished";

    // Request version 1 (patch) at offset 1
    TripleIterator* it2 = controller.get(Triple("", "", ""), 1, 1);

    ASSERT_EQ(true, it2->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <c>.", t.to_string()) << "Element is incorrect";

    ASSERT_EQ(false, it2->next(&t)) << "Iterator should be finished";

    // Request version 0 (snapshot) for ? ? <c>
    TripleIterator* it3 = controller.get(Triple("", "", "<c>"), 0, 0);

    ASSERT_EQ(true, it3->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <c>.", t.to_string()) << "Element is incorrect";

    ASSERT_EQ(false, it3->next(&t)) << "Iterator should be finished";

    // Request version 1 (patch) for ? ? <c>
    TripleIterator* it4 = controller.get(Triple("", "", "<c>"), 0, 1);

    ASSERT_EQ(true, it4->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <c>.", t.to_string()) << "Element is incorrect";

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

    Patch patch1;
    patch1.add(PatchElement(Triple("g", "p", "o"), false));
    patch1.add(PatchElement(Triple("a", "p", "o"), true));
    controller.append(patch1, 1);

    Patch patch2;
    patch2.add(PatchElement(Triple("s", "z", "o"), false));
    patch2.add(PatchElement(Triple("s", "a", "o"), true));
    controller.append(patch2, 1);

    Patch patch3;
    patch3.add(PatchElement(Triple("g", "p", "o"), false));
    patch3.add(PatchElement(Triple("a", "p", "o"), false));
    patch3.add(PatchElement(Triple("h", "z", "o"), false));
    patch3.add(PatchElement(Triple("l", "a", "o"), true));
    controller.append(patch3, 2);

    Patch patch4;
    patch4.add(PatchElement(Triple("h", "p", "o"), false));
    patch4.add(PatchElement(Triple("s", "z", "o"), false));
    controller.append(patch4, 4);

    Patch patch5;
    patch5.add(PatchElement(Triple("h", "p", "o"), true));
    controller.append(patch5, 5);

    Patch patch6;
    patch6.add(PatchElement(Triple("a", "p", "o"), true));
    controller.append(patch6, 6);

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
    TripleIterator* it0 = controller.get(Triple("", "", ""), 0, 0);

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("g p o.", t.to_string()) << "Element is incorrect";

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("h p o.", t.to_string()) << "Element is incorrect";

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("h z o.", t.to_string()) << "Element is incorrect";

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("s z o.", t.to_string()) << "Element is incorrect";

    ASSERT_EQ(false, it0->next(&t)) << "Iterator should be finished";

    // Request version 1 (patch)
    TripleIterator* it1 = controller.get(Triple("", "", ""), 0, 1);

    ASSERT_EQ(true, it1->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("h p o.", t.to_string()) << "Element is incorrect";

    ASSERT_EQ(true, it1->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("h z o.", t.to_string()) << "Element is incorrect";

    ASSERT_EQ(true, it1->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("a p o.", t.to_string()) << "Element is incorrect";

    ASSERT_EQ(true, it1->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", t.to_string()) << "Element is incorrect";

    ASSERT_EQ(false, it1->next(&t)) << "Iterator should be finished";

    // Request version 1 (patch), offset 1
    TripleIterator* it2 = controller.get(Triple("", "", ""), 1, 1);

    ASSERT_EQ(true, it2->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("h z o.", t.to_string()) << "Element is incorrect";

    ASSERT_EQ(true, it2->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("a p o.", t.to_string()) << "Element is incorrect";

    ASSERT_EQ(true, it2->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", t.to_string()) << "Element is incorrect";

    ASSERT_EQ(false, it2->next(&t)) << "Iterator should be finished";

    // Request version 1 (patch), offset 2
    TripleIterator* it3 = controller.get(Triple("", "", ""), 2, 1);

    ASSERT_EQ(true, it3->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("a p o.", t.to_string()) << "Element is incorrect";

    ASSERT_EQ(true, it3->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", t.to_string()) << "Element is incorrect";

    ASSERT_EQ(false, it3->next(&t)) << "Iterator should be finished";

    // Request version 1 (patch), offset 3
    TripleIterator* it4 = controller.get(Triple("", "", ""), 3, 1);

    ASSERT_EQ(true, it4->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", t.to_string()) << "Element is incorrect";

    ASSERT_EQ(false, it4->next(&t)) << "Iterator should be finished";

    // Request version 1 (patch), offset 4
    TripleIterator* it5 = controller.get(Triple("", "", ""), 4, 1);

    ASSERT_EQ(false, it5->next(&t)) << "Iterator should be finished";

    // Request version 2 (patch)
    TripleIterator* it6 = controller.get(Triple("", "", ""), 0, 2);

    ASSERT_EQ(true, it6->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("h p o.", t.to_string()) << "Element is incorrect";

    ASSERT_EQ(true, it6->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("l a o.", t.to_string()) << "Element is incorrect";

    ASSERT_EQ(true, it6->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", t.to_string()) << "Element is incorrect";

    ASSERT_EQ(false, it6->next(&t)) << "Iterator should be finished";

    // Request version 2 (patch), offset 1
    TripleIterator* it7 = controller.get(Triple("", "", ""), 1, 2);

    ASSERT_EQ(true, it7->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("l a o.", t.to_string()) << "Element is incorrect";

    ASSERT_EQ(true, it7->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", t.to_string()) << "Element is incorrect";

    ASSERT_EQ(false, it7->next(&t)) << "Iterator should be finished";

    // Request version 2 (patch), offset 2
    TripleIterator* it8 = controller.get(Triple("", "", ""), 2, 2);

    ASSERT_EQ(true, it8->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", t.to_string()) << "Element is incorrect";

    ASSERT_EQ(false, it8->next(&t)) << "Iterator should be finished";

    // Request version 2 (patch), offset 3
    TripleIterator* it9 = controller.get(Triple("", "", ""), 3, 2);

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

    Patch patch1;
    patch1.add(PatchElement(Triple("g", "p", "o"), false));
    patch1.add(PatchElement(Triple("a", "p", "o"), true));
    controller.append(patch1, 1);

    Patch patch2;
    patch2.add(PatchElement(Triple("s", "z", "o"), false));
    patch2.add(PatchElement(Triple("s", "a", "o"), true));
    controller.append(patch2, 1);

    Patch patch3;
    patch3.add(PatchElement(Triple("g", "p", "o"), false));
    patch3.add(PatchElement(Triple("a", "p", "o"), false));
    patch3.add(PatchElement(Triple("h", "z", "o"), false));
    patch3.add(PatchElement(Triple("l", "a", "o"), true));
    controller.append(patch3, 2);

    Patch patch4;
    patch4.add(PatchElement(Triple("h", "p", "o"), false));
    patch4.add(PatchElement(Triple("s", "z", "o"), false));
    controller.append(patch4, 4);

    Patch patch5;
    patch5.add(PatchElement(Triple("h", "p", "o"), true));
    controller.append(patch5, 5);

    Patch patch6;
    patch6.add(PatchElement(Triple("a", "p", "o"), true));
    controller.append(patch6, 6);

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
    TripleIterator* it0 = controller.get(Triple("", "", "o"), 0, 0);

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("g p o.", t.to_string()) << "Element is incorrect";

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("h p o.", t.to_string()) << "Element is incorrect";

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("h z o.", t.to_string()) << "Element is incorrect";

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("s z o.", t.to_string()) << "Element is incorrect";

    ASSERT_EQ(false, it0->next(&t)) << "Iterator should be finished";

    // Request version 1 (patch)
    TripleIterator* it1 = controller.get(Triple("", "", "o"), 0, 1);

    ASSERT_EQ(true, it1->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("h p o.", t.to_string()) << "Element is incorrect";

    ASSERT_EQ(true, it1->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("h z o.", t.to_string()) << "Element is incorrect";

    ASSERT_EQ(true, it1->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("a p o.", t.to_string()) << "Element is incorrect";

    ASSERT_EQ(true, it1->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", t.to_string()) << "Element is incorrect";

    ASSERT_EQ(false, it1->next(&t)) << "Iterator should be finished";

    // Request version 1 (patch), offset 1
    TripleIterator* it2 = controller.get(Triple("", "", "o"), 1, 1);

    ASSERT_EQ(true, it2->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("h z o.", t.to_string()) << "Element is incorrect";

    ASSERT_EQ(true, it2->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("a p o.", t.to_string()) << "Element is incorrect";

    ASSERT_EQ(true, it2->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", t.to_string()) << "Element is incorrect";

    ASSERT_EQ(false, it2->next(&t)) << "Iterator should be finished";

    // Request version 1 (patch), offset 2
    TripleIterator* it3 = controller.get(Triple("", "", "o"), 2, 1);

    ASSERT_EQ(true, it3->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("a p o.", t.to_string()) << "Element is incorrect";

    ASSERT_EQ(true, it3->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", t.to_string()) << "Element is incorrect";

    ASSERT_EQ(false, it3->next(&t)) << "Iterator should be finished";

    // Request version 1 (patch), offset 3
    TripleIterator* it4 = controller.get(Triple("", "", "o"), 3, 1);

    ASSERT_EQ(true, it4->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", t.to_string()) << "Element is incorrect";

    ASSERT_EQ(false, it4->next(&t)) << "Iterator should be finished";

    // Request version 1 (patch), offset 4
    TripleIterator* it5 = controller.get(Triple("", "", "o"), 4, 1);

    ASSERT_EQ(false, it5->next(&t)) << "Iterator should be finished";

    // Request version 2 (patch)
    TripleIterator* it6 = controller.get(Triple("", "", "o"), 0, 2);

    ASSERT_EQ(true, it6->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("h p o.", t.to_string()) << "Element is incorrect";

    ASSERT_EQ(true, it6->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("l a o.", t.to_string()) << "Element is incorrect";

    ASSERT_EQ(true, it6->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", t.to_string()) << "Element is incorrect";

    ASSERT_EQ(false, it6->next(&t)) << "Iterator should be finished";

    // Request version 2 (patch), offset 1
    TripleIterator* it7 = controller.get(Triple("", "", "o"), 1, 2);

    ASSERT_EQ(true, it7->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("l a o.", t.to_string()) << "Element is incorrect";

    ASSERT_EQ(true, it7->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", t.to_string()) << "Element is incorrect";

    ASSERT_EQ(false, it7->next(&t)) << "Iterator should be finished";

    // Request version 2 (patch), offset 2
    TripleIterator* it8 = controller.get(Triple("", "", "o"), 2, 2);

    ASSERT_EQ(true, it8->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", t.to_string()) << "Element is incorrect";

    ASSERT_EQ(false, it8->next(&t)) << "Iterator should be finished";

    // Request version 2 (patch), offset 3
    TripleIterator* it9 = controller.get(Triple("", "", "o"), 3, 2);

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

    Patch patch1;
    patch1.add(PatchElement(Triple("g", "p", "o"), false));
    patch1.add(PatchElement(Triple("a", "p", "o"), true));
    controller.append(patch1, 1);

    Patch patch2;
    patch2.add(PatchElement(Triple("s", "z", "o"), false));
    patch2.add(PatchElement(Triple("s", "a", "o"), true));
    controller.append(patch2, 1);

    Patch patch3;
    patch3.add(PatchElement(Triple("g", "p", "o"), false));
    patch3.add(PatchElement(Triple("a", "p", "o"), false));
    patch3.add(PatchElement(Triple("h", "z", "o"), false));
    patch3.add(PatchElement(Triple("l", "a", "o"), true));
    controller.append(patch3, 2);

    Patch patch4;
    patch4.add(PatchElement(Triple("h", "p", "o"), false));
    patch4.add(PatchElement(Triple("s", "z", "o"), false));
    controller.append(patch4, 4);

    Patch patch5;
    patch5.add(PatchElement(Triple("h", "p", "o"), true));
    controller.append(patch5, 5);

    Patch patch6;
    patch6.add(PatchElement(Triple("a", "p", "o"), true));
    controller.append(patch6, 6);

    // --- filtered by s ? ?

    // Expected version 0:
    // s z o

    // Expected version 1:
    // s a o (+)

    // Expected version 2:
    // s a o (+)

    Triple t;

    // Request version 0 (snapshot)
    TripleIterator* it0 = controller.get(Triple("s", "", ""), 0, 0);

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("s z o.", t.to_string()) << "Element is incorrect";

    ASSERT_EQ(false, it0->next(&t)) << "Iterator should be finished";

    // Request version 1 (patch)
    TripleIterator* it1 = controller.get(Triple("s", "", ""), 0, 1);

    ASSERT_EQ(true, it1->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", t.to_string()) << "Element is incorrect";

    ASSERT_EQ(false, it1->next(&t)) << "Iterator should be finished";

    // Request version 1 (patch), offset 1
    TripleIterator* it2 = controller.get(Triple("s", "", ""), 1, 1);

    ASSERT_EQ(false, it2->next(&t)) << "Iterator should be finished";

    // Request version 2 (patch)
    TripleIterator* it6 = controller.get(Triple("s", "", ""), 0, 2);

    ASSERT_EQ(true, it6->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", t.to_string()) << "Element is incorrect";

    ASSERT_EQ(false, it6->next(&t)) << "Iterator should be finished";

    // Request version 2 (patch), offset 1
    TripleIterator* it7 = controller.get(Triple("s", "", ""), 1, 2);

    ASSERT_EQ(false, it7->next(&t)) << "Iterator should be finished";
}
