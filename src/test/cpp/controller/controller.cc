#include <gtest/gtest.h>
#include <regex>
#include <dirent.h>

#include "../../../main/cpp/controller/controller.h"
#include "../../../main/cpp/snapshot/vector_triple_iterator.h"

#define BASEURI "<http://example.org>"
#define TESTPATH "./"

// Due to the use of shared_pointer some patch trees can have a "late" deletion
// as it's held by an iterator that gets deleted after the controller
// so the meta file has not been created yet at cleanup (since it is made at patchtree deletion)
// we need to delete them as it can cause problems with later tests
void clean_meta_files() {
    std::regex r("meta_([0-9]+).dat");
    std::smatch base_match;
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir(TESTPATH)) != nullptr) {
        while ((ent = readdir(dir)) != nullptr) {
            std::string dir_name = std::string(ent->d_name);
            if(std::regex_match(dir_name, base_match, r)) {
                std::remove(base_match.str().c_str());
            }
        }
    }
}

// The fixture for testing class controller->
class ControllerTest : public ::testing::Test {
protected:
    Controller* controller;

    ControllerTest() : controller(new Controller(TESTPATH)) {}

    virtual void SetUp() {
        clean_meta_files();
    }

    virtual void TearDown() {
        Controller::cleanup(TESTPATH, controller);
        clean_meta_files();
    }
};

class ControllerMSTest : public ::testing::Test {
protected:
    Controller* controller;
    SnapshotCreationStrategy* strategy;

    ControllerMSTest() {
        // Patch every 2 versions
        // 0 (snapshot), 1 (patch), 2 (snapshot), 3 (patch), 4 (snapshot), ...
        strategy = new CreateSnapshotEveryN(2);
        controller = new Controller(TESTPATH, strategy);
    }

    virtual void SetUp() {

    }

    virtual void TearDown() {
        Controller::cleanup(TESTPATH, controller);
        clean_meta_files();
    }
};

class ControllerMSTest2 : public ::testing::Test {
protected:
    Controller* controller;
    SnapshotCreationStrategy* strategy;

    ControllerMSTest2() {
        // Patch every 2 versions
        // 0 (snapshot), 1 (patch), 2 (patch), 3 (snapshot), 4 (patch), ...
        strategy = new CreateSnapshotEveryN(3);
        controller = new Controller(TESTPATH, strategy);
    }

    virtual void SetUp() {

    }

    virtual void TearDown() {
        Controller::cleanup(TESTPATH, controller);
        clean_meta_files();
    }
};

TEST_F(ControllerTest, GetEdge) {
    Triple t;

    // Build a snapshot
    std::vector<hdt::TripleString> triples;
    triples.push_back(hdt::TripleString("<a>", "<a>", "<a>"));
    triples.push_back(hdt::TripleString("<a>", "<a>", "<b>"));
    triples.push_back(hdt::TripleString("<a>", "<a>", "<c>"));
    VectorTripleIterator* it = new VectorTripleIterator(triples);
    controller->get_snapshot_manager()->create_snapshot(0, it, BASEURI);
    PatchTreeManager* patchTreeManager = controller->get_patch_tree_manager();

    std::shared_ptr<DictionaryManager> dict = controller->get_snapshot_manager()->get_dictionary_manager(0);

    // Request version 1 (after snapshot before a patch id added)
    TripleIterator* it1 = controller->get_version_materialized(Triple("", "", "", dict), 0, 1);

    ASSERT_EQ(true, it1->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <a>.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it1->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <b>.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it1->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <c>.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(false, it1->next(&t)) << "Iterator should be finished";

    // Apply a simple patch
    PatchSorted patch1(dict);
    patch1.add(PatchElement(Triple("<a>", "<a>", "<b>", dict), false));
    patchTreeManager->append(patch1, 1, dict);

    // Request version -1 (before first snapshot)
    TripleIterator* it0 = controller->get_version_materialized(Triple("", "", "", dict), 0, -1);
    ASSERT_EQ(false, it0->next(&t)) << "Iterator should be empty";

    // Request version 2 (after last patch)
    TripleIterator* it2 = controller->get_version_materialized(Triple("", "", "", dict), 0, 2);

    ASSERT_EQ(true, it2->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <a>.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it2->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <c>.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(false, it2->next(&t)) << "Iterator should be finished";
}

TEST_F(ControllerTest, PatchBuilder) {
    controller->new_patch_bulk()
            ->addition(hdt::TripleString("a", "a", "a"))
            ->addition(hdt::TripleString("b", "b", "b"))
            ->addition(hdt::TripleString("c", "c", "c"))
            ->commit();

    controller->new_patch_bulk()
            ->deletion(hdt::TripleString("a", "a", "a"))
            ->addition(hdt::TripleString("d", "d", "d"))
            ->deletion(hdt::TripleString("c", "c", "c"))
            ->commit();

    std::shared_ptr<DictionaryManager> dict = controller->get_snapshot_manager()->get_dictionary_manager(0);
    ASSERT_EQ(3, controller->get_version_materialized_count(Triple("", "", "", dict), 0).first) << "Count is incorrect";
    ASSERT_EQ(2, controller->get_version_materialized_count(Triple("", "", "", dict), 1).first) << "Count is incorrect";
}

TEST_F(ControllerTest, PatchBuilderStreaming) {
    controller->new_patch_stream()
            ->addition(hdt::TripleString("a", "a", "a"))
            ->addition(hdt::TripleString("b", "b", "b"))
            ->addition(hdt::TripleString("c", "c", "c"))
            ->close();

    controller->new_patch_stream()
            ->deletion(hdt::TripleString("a", "a", "a"))
            ->addition(hdt::TripleString("d", "d", "d"))
            ->deletion(hdt::TripleString("c", "c", "c"))
            ->close();

    std::shared_ptr<DictionaryManager> dict = controller->get_snapshot_manager()->get_dictionary_manager(0);
    ASSERT_EQ(3, controller->get_version_materialized_count(Triple("", "", "", dict), 0).first) << "Count is incorrect";
    ASSERT_EQ(2, controller->get_version_materialized_count(Triple("", "", "", dict), 1).first) << "Count is incorrect";
}

TEST_F(ControllerTest, GetVersionMaterializedSimple) {
    // Build a snapshot
    std::vector<hdt::TripleString> triples;
    triples.push_back(hdt::TripleString("<a>", "<a>", "<a>"));
    triples.push_back(hdt::TripleString("<a>", "<a>", "<b>"));
    triples.push_back(hdt::TripleString("<a>", "<a>", "<c>"));
    VectorTripleIterator* it = new VectorTripleIterator(triples);
    controller->get_snapshot_manager()->create_snapshot(0, it, BASEURI);
    PatchTreeManager* patchTreeManager = controller->get_patch_tree_manager();
    std::shared_ptr<DictionaryManager> dict = controller->get_snapshot_manager()->get_dictionary_manager(0);

    // Apply a simple patch
    PatchSorted patch1(dict);
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
    ASSERT_EQ(3, controller->get_version_materialized_count(Triple("", "", "", dict), 0).first) << "Count is incorrect";
    TripleIterator* it0 = controller->get_version_materialized(Triple("", "", "", dict), 0, 0);

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <a>.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <b>.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <c>.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(false, it0->next(&t)) << "Iterator should be finished";

    // Request version 1 (patch)
    ASSERT_EQ(2, controller->get_version_materialized_count(Triple("", "", "", dict), 1).first) << "Count is incorrect";
    TripleIterator* it1 = controller->get_version_materialized(Triple("", "", "", dict), 0, 1);

    ASSERT_EQ(true, it1->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <a>.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it1->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <c>.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(false, it1->next(&t)) << "Iterator should be finished";

    // Request version 1 (patch) at offset 1
    ASSERT_EQ(2, controller->get_version_materialized_count(Triple("", "", "", dict), 1).first) << "Count is incorrect";
    TripleIterator* it2 = controller->get_version_materialized(Triple("", "", "", dict), 1, 1);

    ASSERT_EQ(true, it2->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <c>.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(false, it2->next(&t)) << "Iterator should be finished";

    // Request version 0 (snapshot) for ? ? <c>

    ASSERT_EQ(1, controller->get_version_materialized_count(Triple("", "", "<c>", dict), 0).first) << "Count is incorrect";
    TripleIterator* it3 = controller->get_version_materialized(Triple("", "", "<c>", dict), 0, 0);

    ASSERT_EQ(true, it3->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <c>.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(false, it3->next(&t)) << "Iterator should be finished";

    // Request version 1 (patch) for ? ? <c>
    ASSERT_EQ(1, controller->get_version_materialized_count(Triple("", "", "<c>", dict), 1).first) << "Count is incorrect";
    TripleIterator* it4 = controller->get_version_materialized(Triple("", "", "<c>", dict), 0, 1);

    ASSERT_EQ(true, it4->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <c>.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(false, it4->next(&t)) << "Iterator should be finished";
}

TEST_F(ControllerTest, GetVersionMaterializedComplex1) {
    // Build a snapshot
    std::vector<hdt::TripleString> triples;
    triples.push_back(hdt::TripleString("g", "p", "o"));
    triples.push_back(hdt::TripleString("s", "z", "o"));
    triples.push_back(hdt::TripleString("h", "z", "o"));
    triples.push_back(hdt::TripleString("h", "p", "o"));
    VectorTripleIterator* it = new VectorTripleIterator(triples);
    controller->get_snapshot_manager()->create_snapshot(0, it, BASEURI);
    PatchTreeManager* patchTreeManager = controller->get_patch_tree_manager();
    std::shared_ptr<DictionaryManager> dict = controller->get_snapshot_manager()->get_dictionary_manager(0);

    PatchSorted patch1(dict);
    patch1.add(PatchElement(Triple("g", "p", "o", dict), false));
    patch1.add(PatchElement(Triple("a", "p", "o", dict), true));
    patchTreeManager->append(patch1, 1, dict);

    PatchSorted patch2(dict);
    patch2.add(PatchElement(Triple("s", "z", "o", dict), false));
    patch2.add(PatchElement(Triple("s", "a", "o", dict), true));
    patchTreeManager->append(patch2, 1, dict);

    PatchSorted patch3(dict);
    patch3.add(PatchElement(Triple("g", "p", "o", dict), false));
    patch3.add(PatchElement(Triple("a", "p", "o", dict), false));
    patch3.add(PatchElement(Triple("h", "z", "o", dict), false));
    patch3.add(PatchElement(Triple("l", "a", "o", dict), true));
    patchTreeManager->append(patch3, 2, dict);

    PatchSorted patch4(dict);
    patch4.add(PatchElement(Triple("h", "p", "o", dict), false));
    patch4.add(PatchElement(Triple("s", "z", "o", dict), false));
    patchTreeManager->append(patch4, 4, dict);

    PatchSorted patch5(dict);
    patch5.add(PatchElement(Triple("h", "p", "o", dict), true));
    patchTreeManager->append(patch5, 5, dict);

    PatchSorted patch6(dict);
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
    ASSERT_EQ(4, controller->get_version_materialized_count(Triple("", "", "", dict), 0).first) << "Count is incorrect";
    TripleIterator* it0 = controller->get_version_materialized(Triple("", "", "", dict), 0, 0);

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
    ASSERT_EQ(4, controller->get_version_materialized_count(Triple("", "", "", dict), 1).first) << "Count is incorrect";
    TripleIterator* it1 = controller->get_version_materialized(Triple("", "", "", dict), 0, 1);

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
    TripleIterator* it2 = controller->get_version_materialized(Triple("", "", "", dict), 1, 1);

    ASSERT_EQ(true, it2->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("h z o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it2->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("a p o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it2->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(false, it2->next(&t)) << "Iterator should be finished";

    // Request version 1 (patch), offset 2
    TripleIterator* it3 = controller->get_version_materialized(Triple("", "", "", dict), 2, 1);

    ASSERT_EQ(true, it3->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("a p o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it3->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(false, it3->next(&t)) << "Iterator should be finished";

    // Request version 1 (patch), offset 3
    TripleIterator* it4 = controller->get_version_materialized(Triple("", "", "", dict), 3, 1);

    ASSERT_EQ(true, it4->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(false, it4->next(&t)) << "Iterator should be finished";

    // Request version 1 (patch), offset 4
    TripleIterator* it5 = controller->get_version_materialized(Triple("", "", "", dict), 4, 1);

    ASSERT_EQ(false, it5->next(&t)) << "Iterator should be finished";

    // Request version 2 (patch)
    ASSERT_EQ(3, controller->get_version_materialized_count(Triple("", "", "", dict), 2).first) << "Count is incorrect";
    TripleIterator* it6 = controller->get_version_materialized(Triple("", "", "", dict), 0, 2);

    ASSERT_EQ(true, it6->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("h p o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it6->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("l a o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it6->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(false, it6->next(&t)) << "Iterator should be finished";

    // Request version 2 (patch), offset 1
    TripleIterator* it7 = controller->get_version_materialized(Triple("", "", "", dict), 1, 2);

    ASSERT_EQ(true, it7->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("l a o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it7->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(false, it7->next(&t)) << "Iterator should be finished";

    // Request version 2 (patch), offset 2
    TripleIterator* it8 = controller->get_version_materialized(Triple("", "", "", dict), 2, 2);

    ASSERT_EQ(true, it8->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(false, it8->next(&t)) << "Iterator should be finished";

    // Request version 2 (patch), offset 3
    TripleIterator* it9 = controller->get_version_materialized(Triple("", "", "", dict), 3, 2);

    ASSERT_EQ(false, it9->next(&t)) << "Iterator should be finished";
}


TEST_F(ControllerMSTest, GetVersionMaterializedComplexMS1) {
    // Build a snapshot
    std::vector<hdt::TripleString> triples;
    triples.push_back(hdt::TripleString("g", "p", "o"));
    triples.push_back(hdt::TripleString("s", "z", "o"));
    triples.push_back(hdt::TripleString("h", "z", "o"));
    triples.push_back(hdt::TripleString("h", "p", "o"));
    VectorTripleIterator* it = new VectorTripleIterator(triples);
    controller->get_snapshot_manager()->create_snapshot(0, it, BASEURI);
    PatchTreeManager* patchTreeManager = controller->get_patch_tree_manager();
    std::shared_ptr<DictionaryManager> dict = controller->get_snapshot_manager()->get_dictionary_manager(0);

    PatchSorted patch1(dict);
    patch1.add(PatchElement(Triple("g", "p", "o", dict), false));
    patch1.add(PatchElement(Triple("a", "p", "o", dict), true));
    controller->append(patch1, 1, dict);

    PatchSorted patch2(dict);
    patch2.add(PatchElement(Triple("s", "z", "o", dict), false));
    patch2.add(PatchElement(Triple("s", "a", "o", dict), true));
    controller->append(patch2, 1, dict);

    int snapshot_id = controller->get_snapshot_manager()->get_latest_snapshot(1);
    controller->get_snapshot_manager()->load_snapshot(snapshot_id);
    dict = controller->get_snapshot_manager()->get_dictionary_manager(snapshot_id);

    PatchSorted patch3(dict);
    patch3.add(PatchElement(Triple("g", "p", "o", dict), false));
    patch3.add(PatchElement(Triple("a", "p", "o", dict), false));
    patch3.add(PatchElement(Triple("h", "z", "o", dict), false));
    patch3.add(PatchElement(Triple("l", "a", "o", dict), true));
    controller->append(patch3, 2, dict);

    snapshot_id = controller->get_snapshot_manager()->get_latest_snapshot(2);
    controller->get_snapshot_manager()->load_snapshot(snapshot_id);
    dict = controller->get_snapshot_manager()->get_dictionary_manager(snapshot_id);

    PatchSorted patch4(dict);
    patch4.add(PatchElement(Triple("h", "p", "o", dict), false));
    patch4.add(PatchElement(Triple("s", "z", "o", dict), false));
    controller->append(patch4, 4, dict);

    snapshot_id = controller->get_snapshot_manager()->get_latest_snapshot(4);
    controller->get_snapshot_manager()->load_snapshot(snapshot_id);
    dict = controller->get_snapshot_manager()->get_dictionary_manager(snapshot_id);

    PatchSorted patch5(dict);
    patch5.add(PatchElement(Triple("h", "p", "o", dict), true));
    controller->append(patch5, 5, dict);

    snapshot_id = controller->get_snapshot_manager()->get_latest_snapshot(5);
    controller->get_snapshot_manager()->load_snapshot(snapshot_id);
    dict = controller->get_snapshot_manager()->get_dictionary_manager(snapshot_id);

    PatchSorted patch6(dict);
    patch6.add(PatchElement(Triple("a", "p", "o", dict), true));
    controller->append(patch6, 6, dict);

    Triple t;

    // Request version 0 (snapshot)
    dict = controller->get_snapshot_manager()->get_dictionary_manager(0);
    ASSERT_EQ(4, controller->get_version_materialized_count(Triple("", "", "", dict), 0).first) << "Count is incorrect";
    TripleIterator* it0 = controller->get_version_materialized(Triple("", "", "", dict), 0, 0);

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
    snapshot_id = controller->get_snapshot_manager()->get_latest_snapshot(1);
    dict = controller->get_snapshot_manager()->get_dictionary_manager(snapshot_id);
    ASSERT_EQ(4, controller->get_version_materialized_count(Triple("", "", "", dict), 1).first) << "Count is incorrect";
    TripleIterator* it1 = controller->get_version_materialized(Triple("", "", "", dict), 0, 1);

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
    TripleIterator* it2 = controller->get_version_materialized(Triple("", "", "", dict), 1, 1);

    ASSERT_EQ(true, it2->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("h z o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it2->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("a p o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it2->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(false, it2->next(&t)) << "Iterator should be finished";

    // Request version 1 (patch), offset 2
    TripleIterator* it3 = controller->get_version_materialized(Triple("", "", "", dict), 2, 1);

    ASSERT_EQ(true, it3->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("a p o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it3->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(false, it3->next(&t)) << "Iterator should be finished";

    // Request version 1 (patch), offset 3
    TripleIterator* it4 = controller->get_version_materialized(Triple("", "", "", dict), 3, 1);

    ASSERT_EQ(true, it4->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(false, it4->next(&t)) << "Iterator should be finished";

    // Request version 1 (patch), offset 4
    TripleIterator* it5 = controller->get_version_materialized(Triple("", "", "", dict), 4, 1);

    ASSERT_EQ(false, it5->next(&t)) << "Iterator should be finished";

    // Request version 2 (patch)
    snapshot_id = controller->get_snapshot_manager()->get_latest_snapshot(2);
    dict = controller->get_snapshot_manager()->get_dictionary_manager(snapshot_id);
    ASSERT_EQ(3, controller->get_version_materialized_count(Triple("", "", "", dict), 2).first) << "Count is incorrect";
    TripleIterator* it6 = controller->get_version_materialized(Triple("", "", "", dict), 0, 2);

    ASSERT_EQ(true, it6->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("h p o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it6->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("l a o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it6->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(false, it6->next(&t)) << "Iterator should be finished";

    // Request version 2 (patch), offset 1
    TripleIterator* it7 = controller->get_version_materialized(Triple("", "", "", dict), 1, 2);

    ASSERT_EQ(true, it7->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("l a o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it7->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(false, it7->next(&t)) << "Iterator should be finished";

    // Request version 2 (patch), offset 2
    TripleIterator* it8 = controller->get_version_materialized(Triple("", "", "", dict), 2, 2);

    ASSERT_EQ(true, it8->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(false, it8->next(&t)) << "Iterator should be finished";

    // Request version 2 (patch), offset 3
    TripleIterator* it9 = controller->get_version_materialized(Triple("", "", "", dict), 3, 2);

    ASSERT_EQ(false, it9->next(&t)) << "Iterator should be finished";

    // Request version 5, offset 0
    snapshot_id = controller->get_snapshot_manager()->get_latest_snapshot(5);
    dict = controller->get_snapshot_manager()->get_dictionary_manager(snapshot_id);
    TripleIterator* it10 = controller->get_version_materialized(Triple("", "", "", dict), 0, 5);

    ASSERT_EQ(true, it10->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("l a o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it10->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it10->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("h p o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(false, it10->next(&t)) << "Iterator should be finished";

}

TEST_F(ControllerTest, GetVersionMaterializedComplex2) {
    // Build a snapshot
    std::vector<hdt::TripleString> triples;
    triples.push_back(hdt::TripleString("g", "p", "o"));
    triples.push_back(hdt::TripleString("s", "z", "o"));
    triples.push_back(hdt::TripleString("h", "z", "o"));
    triples.push_back(hdt::TripleString("h", "p", "o"));
    VectorTripleIterator* it = new VectorTripleIterator(triples);
    controller->get_snapshot_manager()->create_snapshot(0, it, BASEURI);
    PatchTreeManager* patchTreeManager = controller->get_patch_tree_manager();
    std::shared_ptr<DictionaryManager> dict = controller->get_snapshot_manager()->get_dictionary_manager(0);

    PatchSorted patch1(dict);
    patch1.add(PatchElement(Triple("g", "p", "o", dict), false));
    patch1.add(PatchElement(Triple("a", "p", "o", dict), true));
    patchTreeManager->append(patch1, 1, dict);

    PatchSorted patch2(dict);
    patch2.add(PatchElement(Triple("s", "z", "o", dict), false));
    patch2.add(PatchElement(Triple("s", "a", "o", dict), true));
    patchTreeManager->append(patch2, 1, dict);

    PatchSorted patch3(dict);
    patch3.add(PatchElement(Triple("g", "p", "o", dict), false));
    patch3.add(PatchElement(Triple("a", "p", "o", dict), false));
    patch3.add(PatchElement(Triple("h", "z", "o", dict), false));
    patch3.add(PatchElement(Triple("l", "a", "o", dict), true));
    patchTreeManager->append(patch3, 2, dict);

    PatchSorted patch4(dict);
    patch4.add(PatchElement(Triple("h", "p", "o", dict), false));
    patch4.add(PatchElement(Triple("s", "z", "o", dict), false));
    patchTreeManager->append(patch4, 4, dict);

    PatchSorted patch5(dict);
    patch5.add(PatchElement(Triple("h", "p", "o", dict), true));
    patchTreeManager->append(patch5, 5, dict);

    PatchSorted patch6(dict);
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
    ASSERT_EQ(4, controller->get_version_materialized_count(Triple("", "", "o", dict), 0).first) << "Count is incorrect";
    TripleIterator* it0 = controller->get_version_materialized(Triple("", "", "o", dict), 0, 0);

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
    ASSERT_EQ(4, controller->get_version_materialized_count(Triple("", "", "o", dict), 1).first) << "Count is incorrect";
    TripleIterator* it1 = controller->get_version_materialized(Triple("", "", "o", dict), 0, 1);

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
    TripleIterator* it2 = controller->get_version_materialized(Triple("", "", "o", dict), 1, 1);

    ASSERT_EQ(true, it2->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("h z o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it2->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("a p o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it2->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(false, it2->next(&t)) << "Iterator should be finished";

    // Request version 1 (patch), offset 2
    TripleIterator* it3 = controller->get_version_materialized(Triple("", "", "o", dict), 2, 1);

    ASSERT_EQ(true, it3->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("a p o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it3->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(false, it3->next(&t)) << "Iterator should be finished";

    // Request version 1 (patch), offset 3
    TripleIterator* it4 = controller->get_version_materialized(Triple("", "", "o", dict), 3, 1);

    ASSERT_EQ(true, it4->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(false, it4->next(&t)) << "Iterator should be finished";

    // Request version 1 (patch), offset 4
    TripleIterator* it5 = controller->get_version_materialized(Triple("", "", "o", dict), 4, 1);

    ASSERT_EQ(false, it5->next(&t)) << "Iterator should be finished";

    // Request version 2 (patch)
    ASSERT_EQ(3, controller->get_version_materialized_count(Triple("", "", "o", dict), 2).first) << "Count is incorrect";
    TripleIterator* it6 = controller->get_version_materialized(Triple("", "", "o", dict), 0, 2);

    ASSERT_EQ(true, it6->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("h p o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it6->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("l a o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it6->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(false, it6->next(&t)) << "Iterator should be finished";

    // Request version 2 (patch), offset 1
    TripleIterator* it7 = controller->get_version_materialized(Triple("", "", "o", dict), 1, 2);

    ASSERT_EQ(true, it7->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("l a o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it7->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(false, it7->next(&t)) << "Iterator should be finished";

    // Request version 2 (patch), offset 2
    TripleIterator* it8 = controller->get_version_materialized(Triple("", "", "o", dict), 2, 2);

    ASSERT_EQ(true, it8->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(false, it8->next(&t)) << "Iterator should be finished";

    // Request version 2 (patch), offset 3
    TripleIterator* it9 = controller->get_version_materialized(Triple("", "", "o", dict), 3, 2);

    ASSERT_EQ(false, it9->next(&t)) << "Iterator should be finished";
}


TEST_F(ControllerMSTest, GetVersionMaterializedComplexMS2) {
    // Build a snapshot
    std::vector<hdt::TripleString> triples;
    triples.push_back(hdt::TripleString("g", "p", "o"));
    triples.push_back(hdt::TripleString("s", "z", "o"));
    triples.push_back(hdt::TripleString("h", "z", "o"));
    triples.push_back(hdt::TripleString("h", "p", "o"));
    VectorTripleIterator* it = new VectorTripleIterator(triples);
    controller->get_snapshot_manager()->create_snapshot(0, it, BASEURI);
    std::shared_ptr<DictionaryManager> dict = controller->get_snapshot_manager()->get_dictionary_manager(0);

    PatchSorted patch1(dict);
    patch1.add(PatchElement(Triple("g", "p", "o", dict), false));
    patch1.add(PatchElement(Triple("a", "p", "o", dict), true));
    controller->append(patch1, 1, dict);

    PatchSorted patch2(dict);
    patch2.add(PatchElement(Triple("s", "z", "o", dict), false));
    patch2.add(PatchElement(Triple("s", "a", "o", dict), true));
    controller->append(patch2, 1, dict);

    int snapshot_id = controller->get_snapshot_manager()->get_latest_snapshot(1);
    controller->get_snapshot_manager()->load_snapshot(snapshot_id);
    dict = controller->get_snapshot_manager()->get_dictionary_manager(snapshot_id);

    PatchSorted patch3(dict);
    patch3.add(PatchElement(Triple("g", "p", "o", dict), false));
    patch3.add(PatchElement(Triple("a", "p", "o", dict), false));
    patch3.add(PatchElement(Triple("h", "z", "o", dict), false));
    patch3.add(PatchElement(Triple("l", "a", "o", dict), true));
    controller->append(patch3, 2, dict);

    snapshot_id = controller->get_snapshot_manager()->get_latest_snapshot(2);
    controller->get_snapshot_manager()->load_snapshot(snapshot_id);
    dict = controller->get_snapshot_manager()->get_dictionary_manager(snapshot_id);

    PatchSorted patch4(dict);
    patch4.add(PatchElement(Triple("h", "p", "o", dict), false));
    patch4.add(PatchElement(Triple("s", "z", "o", dict), false));
    controller->append(patch4, 4, dict);

    snapshot_id = controller->get_snapshot_manager()->get_latest_snapshot(4);
    controller->get_snapshot_manager()->load_snapshot(snapshot_id);
    dict = controller->get_snapshot_manager()->get_dictionary_manager(snapshot_id);

    PatchSorted patch5(dict);
    patch5.add(PatchElement(Triple("h", "p", "o", dict), true));
    controller->append(patch5, 5, dict);

    snapshot_id = controller->get_snapshot_manager()->get_latest_snapshot(5);
    controller->get_snapshot_manager()->load_snapshot(snapshot_id);
    dict = controller->get_snapshot_manager()->get_dictionary_manager(snapshot_id);

    PatchSorted patch6(dict);
    patch6.add(PatchElement(Triple("a", "p", "o", dict), true));
    controller->append(patch6, 6, dict);


    Triple t;

    // Request version 0 (snapshot)
    snapshot_id = controller->get_snapshot_manager()->get_latest_snapshot(0);
    dict = controller->get_snapshot_manager()->get_dictionary_manager(snapshot_id);

    ASSERT_EQ(4, controller->get_version_materialized_count(Triple("", "", "o", dict), 0).first) << "Count is incorrect";
    TripleIterator* it0 = controller->get_version_materialized(Triple("", "", "o", dict), 0, 0);

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
    snapshot_id = controller->get_snapshot_manager()->get_latest_snapshot(1);
    dict = controller->get_snapshot_manager()->get_dictionary_manager(snapshot_id);

    ASSERT_EQ(4, controller->get_version_materialized_count(Triple("", "", "o", dict), 1).first) << "Count is incorrect";
    TripleIterator* it1 = controller->get_version_materialized(Triple("", "", "o", dict), 0, 1);

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
    TripleIterator* it2 = controller->get_version_materialized(Triple("", "", "o", dict), 1, 1);

    ASSERT_EQ(true, it2->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("h z o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it2->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("a p o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it2->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(false, it2->next(&t)) << "Iterator should be finished";

    // Request version 1 (patch), offset 2
    TripleIterator* it3 = controller->get_version_materialized(Triple("", "", "o", dict), 2, 1);

    ASSERT_EQ(true, it3->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("a p o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it3->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(false, it3->next(&t)) << "Iterator should be finished";

    // Request version 1 (patch), offset 3
    TripleIterator* it4 = controller->get_version_materialized(Triple("", "", "o", dict), 3, 1);

    ASSERT_EQ(true, it4->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(false, it4->next(&t)) << "Iterator should be finished";

    // Request version 1 (patch), offset 4
    TripleIterator* it5 = controller->get_version_materialized(Triple("", "", "o", dict), 4, 1);

    ASSERT_EQ(false, it5->next(&t)) << "Iterator should be finished";

    // Request version 2 (patch)
    snapshot_id = controller->get_snapshot_manager()->get_latest_snapshot(2);
    dict = controller->get_snapshot_manager()->get_dictionary_manager(snapshot_id);

    ASSERT_EQ(3, controller->get_version_materialized_count(Triple("", "", "o", dict), 2).first) << "Count is incorrect";
    TripleIterator* it6 = controller->get_version_materialized(Triple("", "", "o", dict), 0, 2);

    ASSERT_EQ(true, it6->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("l a o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it6->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it6->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("h p o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(false, it6->next(&t)) << "Iterator should be finished";

    // Request version 2 (patch), offset 1
    TripleIterator* it7 = controller->get_version_materialized(Triple("", "", "o", dict), 1, 2);

    ASSERT_EQ(true, it7->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it7->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("h p o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(false, it7->next(&t)) << "Iterator should be finished";

    // Request version 2 (patch), offset 2
    TripleIterator* it8 = controller->get_version_materialized(Triple("", "", "o", dict), 2, 2);

    ASSERT_EQ(true, it8->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("h p o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(false, it8->next(&t)) << "Iterator should be finished";

    // Request version 2 (patch), offset 3
    TripleIterator* it9 = controller->get_version_materialized(Triple("", "", "o", dict), 3, 2);

    ASSERT_EQ(false, it9->next(&t)) << "Iterator should be finished";
}

TEST_F(ControllerTest, GetVersionMaterializedComplex3) {
    // Build a snapshot
    std::vector<hdt::TripleString> triples;
    triples.push_back(hdt::TripleString("g", "p", "o"));
    triples.push_back(hdt::TripleString("s", "z", "o"));
    triples.push_back(hdt::TripleString("h", "z", "o"));
    triples.push_back(hdt::TripleString("h", "p", "o"));
    VectorTripleIterator* it = new VectorTripleIterator(triples);
    controller->get_snapshot_manager()->create_snapshot(0, it, BASEURI);
    PatchTreeManager* patchTreeManager = controller->get_patch_tree_manager();
    std::shared_ptr<DictionaryManager> dict = controller->get_snapshot_manager()->get_dictionary_manager(0);

    PatchSorted patch1(dict);
    patch1.add(PatchElement(Triple("g", "p", "o", dict), false));
    patch1.add(PatchElement(Triple("a", "p", "o", dict), true));
    patchTreeManager->append(patch1, 1, dict);

    PatchSorted patch2(dict);
    patch2.add(PatchElement(Triple("s", "z", "o", dict), false));
    patch2.add(PatchElement(Triple("s", "a", "o", dict), true));
    patchTreeManager->append(patch2, 1, dict);

    PatchSorted patch3(dict);
    patch3.add(PatchElement(Triple("g", "p", "o", dict), false));
    patch3.add(PatchElement(Triple("a", "p", "o", dict), false));
    patch3.add(PatchElement(Triple("h", "z", "o", dict), false));
    patch3.add(PatchElement(Triple("l", "a", "o", dict), true));
    patchTreeManager->append(patch3, 2, dict);

    PatchSorted patch4(dict);
    patch4.add(PatchElement(Triple("h", "p", "o", dict), false));
    patch4.add(PatchElement(Triple("s", "z", "o", dict), false));
    patchTreeManager->append(patch4, 4, dict);

    PatchSorted patch5(dict);
    patch5.add(PatchElement(Triple("h", "p", "o", dict), true));
    patchTreeManager->append(patch5, 5, dict);

    PatchSorted patch6(dict);
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
    ASSERT_EQ(1, controller->get_version_materialized_count(Triple("s", "", "", dict), 0).first) << "Count is incorrect";
    TripleIterator* it0 = controller->get_version_materialized(Triple("s", "", "", dict), 0, 0);

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("s z o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(false, it0->next(&t)) << "Iterator should be finished";

    // Request version 1 (patch)
    ASSERT_EQ(1, controller->get_version_materialized_count(Triple("s", "", "", dict), 1).first) << "Count is incorrect";
    TripleIterator* it1 = controller->get_version_materialized(Triple("s", "", "", dict), 0, 1);

    ASSERT_EQ(true, it1->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(false, it1->next(&t)) << "Iterator should be finished";

    // Request version 1 (patch), offset 1
    TripleIterator* it2 = controller->get_version_materialized(Triple("s", "", "", dict), 1, 1);

    ASSERT_EQ(false, it2->next(&t)) << "Iterator should be finished";

    // Request version 2 (patch)
    ASSERT_EQ(1, controller->get_version_materialized_count(Triple("s", "", "", dict), 2).first) << "Count is incorrect";
    TripleIterator* it6 = controller->get_version_materialized(Triple("s", "", "", dict), 0, 2);

    ASSERT_EQ(true, it6->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("s a o.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(false, it6->next(&t)) << "Iterator should be finished";

    // Request version 2 (patch), offset 1
    TripleIterator* it7 = controller->get_version_materialized(Triple("s", "", "", dict), 1, 2);

    ASSERT_EQ(false, it7->next(&t)) << "Iterator should be finished";
}

TEST_F(ControllerTest, EdgeCaseVersionMaterialized1) {
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
    std::vector<hdt::TripleString> triples;
    triples.push_back(hdt::TripleString("0", "0", "0"));
    triples.push_back(hdt::TripleString("1", "1", "1"));
    triples.push_back(hdt::TripleString("2", "2", "2"));
    triples.push_back(hdt::TripleString("3", "3", "3"));
    triples.push_back(hdt::TripleString("4", "4", "4"));
    triples.push_back(hdt::TripleString("5", "5", "5"));
    triples.push_back(hdt::TripleString("6", "6", "6"));
    triples.push_back(hdt::TripleString("7", "7", "7"));
    triples.push_back(hdt::TripleString("8", "8", "8"));
    triples.push_back(hdt::TripleString("9", "9", "9"));
    VectorTripleIterator *it = new VectorTripleIterator(triples);
    controller->get_snapshot_manager()->create_snapshot(0, it, BASEURI);
    PatchTreeManager* patchTreeManager = controller->get_patch_tree_manager();
    std::shared_ptr<DictionaryManager> dict = controller->get_snapshot_manager()->get_dictionary_manager(0);

    PatchSorted patch1(dict);
    patch1.add(PatchElement(Triple("0", "0", "0", dict), false));
    patch1.add(PatchElement(Triple("1", "1", "1", dict), false));
    patch1.add(PatchElement(Triple("2", "2", "2", dict), false));
    // No 3!
    patch1.add(PatchElement(Triple("4", "4", "4", dict), false));
    patch1.add(PatchElement(Triple("5", "5", "5", dict), false));
    patchTreeManager->append(patch1, 1, dict);

    Triple t;

    // Request version 1, offset 0
    TripleIterator* it0 = controller->get_version_materialized(Triple("", "", "", dict), 0, 1);

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("3 3 3.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("6 6 6.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("7 7 7.", t.to_string(*dict)) << "Element is incorrect";

    // Request version 1, offset 3 (The actual edge case!)
    TripleIterator* it1 = controller->get_version_materialized(Triple("", "", "", dict), 3, 1);

    ASSERT_EQ(true, it1->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("8 8 8.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it1->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("9 9 9.", t.to_string(*dict)) << "Element is incorrect";
}

TEST_F(ControllerTest, EdgeCaseVersionMaterialized2) {
    /*
     * This tests the case in which a deletion that does not match the queried triple pattern
     * exists *before* all applicable deletions, and the first matching triple is just a snapshot triple.
     */

    // Build a snapshot
    std::vector<hdt::TripleString> triples;
    triples.push_back(hdt::TripleString("a", "a", "a"));
    triples.push_back(hdt::TripleString("b", "b", "b"));
    triples.push_back(hdt::TripleString("y", "a", "a"));
    triples.push_back(hdt::TripleString("z", "a", "a"));
    VectorTripleIterator* it = new VectorTripleIterator(triples);
    controller->get_snapshot_manager()->create_snapshot(0, it, BASEURI);
    PatchTreeManager* patchTreeManager = controller->get_patch_tree_manager();
    std::shared_ptr<DictionaryManager> dict = controller->get_snapshot_manager()->get_dictionary_manager(0);

    PatchSorted patch1(dict);
    patch1.add(PatchElement(Triple("b", "b", "b", dict), false));
    patch1.add(PatchElement(Triple("y", "a", "a", dict), false));
    patch1.add(PatchElement(Triple("z", "a", "a", dict), false));
    patchTreeManager->append(patch1, 1, dict);

    // --- filtered by a ? a

    // Expected version 0:
    // a a a
    // y a a
    // z a a

    // Expected version 1:
    // a a a

    Triple t;

    // Request version 0 (snapshot)
    ASSERT_EQ(3, controller->get_version_materialized_count(Triple("", "a", "a", dict), 0).first) << "Count is incorrect";
    TripleIterator* it0 = controller->get_version_materialized(Triple("", "a", "a", dict), 0, 0);

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("a a a.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("y a a.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("z a a.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(false, it0->next(&t)) << "Iterator has a no next value";

    // Request version 1 (patch)
    ASSERT_EQ(1, controller->get_version_materialized_count(Triple("", "a", "a", dict), 1).first) << "Count is incorrect";
    TripleIterator* it1 = controller->get_version_materialized(Triple("", "a", "a", dict), 0, 1);

    ASSERT_EQ(true, it1->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("a a a.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(false, it1->next(&t)) << "Iterator has a no next value";
}

TEST_F(ControllerTest, EdgeCaseVersionMaterialized3) {
    /*
     * Same as the previous case, but we start from an offset.
     */

    // Build a snapshot
    std::vector<hdt::TripleString> triples;
    triples.push_back(hdt::TripleString("a", "a", "a"));
    triples.push_back(hdt::TripleString("b", "a", "a"));
    triples.push_back(hdt::TripleString("c", "c", "c"));
    triples.push_back(hdt::TripleString("d", "a", "a"));
    triples.push_back(hdt::TripleString("y", "a", "a"));
    triples.push_back(hdt::TripleString("z", "a", "a"));
    VectorTripleIterator* it = new VectorTripleIterator(triples);
    controller->get_snapshot_manager()->create_snapshot(0, it, BASEURI);
    PatchTreeManager* patchTreeManager = controller->get_patch_tree_manager();
    std::shared_ptr<DictionaryManager> dict = controller->get_snapshot_manager()->get_dictionary_manager(0);

    PatchSorted patch1(dict);
    patch1.add(PatchElement(Triple("b", "a", "a", dict), false));
    patch1.add(PatchElement(Triple("c", "c", "c", dict), false));
    patch1.add(PatchElement(Triple("y", "a", "a", dict), false));
    patch1.add(PatchElement(Triple("z", "a", "a", dict), false));
    patchTreeManager->append(patch1, 1, dict);

    // --- filtered by a ? a

    // Expected version 0:
    // a a a
    // b a a
    // d a a
    // y a a
    // z a a

    // Expected version 1:
    // a a a

    Triple t;

    // Request version 0 (snapshot)
    ASSERT_EQ(5, controller->get_version_materialized_count(Triple("", "a", "a", dict), 0).first) << "Count is incorrect";
    TripleIterator* it0 = controller->get_version_materialized(Triple("", "a", "a", dict), 0, 0);

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("a a a.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("b a a.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("d a a.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("y a a.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("z a a.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(false, it0->next(&t)) << "Iterator has a no next value";

    // Request version 1 (patch)
    ASSERT_EQ(2, controller->get_version_materialized_count(Triple("", "a", "a", dict), 1).first) << "Count is incorrect";
    TripleIterator* it1 = controller->get_version_materialized(Triple("", "a", "a", dict), 0, 1);

    ASSERT_EQ(true, it1->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("a a a.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it1->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("d a a.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(false, it1->next(&t)) << "Iterator has a no next value";

    // Request version 1 (patch), offset 1
    TripleIterator* it2 = controller->get_version_materialized(Triple("", "a", "a", dict), 1, 1);

    ASSERT_EQ(true, it2->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("d a a.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(false, it2->next(&t)) << "Iterator has a no next value";

    // Request version 1 (patch), offset 2
    TripleIterator* it3 = controller->get_version_materialized(Triple("", "a", "a", dict), 2, 1);

    ASSERT_EQ(false, it3->next(&t)) << "Iterator has a no next value";
}

TEST_F(ControllerTest, VersionMaterializedOrder) {
    /*
     * Same as the previous case, but we start from an offset.
     */

    // Build a snapshot
    std::vector<hdt::TripleString> triples;
    triples.push_back(hdt::TripleString("w", "a", "d"));
    triples.push_back(hdt::TripleString("z", "a", "c"));
    triples.push_back(hdt::TripleString("y", "a", "b"));
    triples.push_back(hdt::TripleString("x", "a", "a"));
    triples.push_back(hdt::TripleString("w", "b", "a"));
    triples.push_back(hdt::TripleString("x", "b", "b"));
    triples.push_back(hdt::TripleString("y", "b", "c"));
    triples.push_back(hdt::TripleString("z", "b", "d"));
    VectorTripleIterator* it = new VectorTripleIterator(triples);
    controller->get_snapshot_manager()->create_snapshot(0, it, BASEURI);
    PatchTreeManager* patchTreeManager = controller->get_patch_tree_manager();
    std::shared_ptr<DictionaryManager> dict = controller->get_snapshot_manager()->get_dictionary_manager(0);

    Triple t;

    // Request version 0 (snapshot)
    ASSERT_EQ(4, controller->get_version_materialized_count(Triple("", "a", "", dict), 0).first) << "Count is incorrect";
    TripleIterator* it0 = controller->get_version_materialized(Triple("", "a", "", dict), 0, 0);

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("w a d.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("x a a.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("y a b.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("z a c.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(false, it0->next(&t)) << "Iterator has a no next value";

    ASSERT_EQ(2, controller->get_version_materialized_count(Triple("", "", "c", dict), 0).first) << "Count is incorrect";
    TripleIterator* it1 = controller->get_version_materialized(Triple("", "", "c", dict), 0, 0);

    ASSERT_EQ(true, it1->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("z a c.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(true, it1->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("y b c.", t.to_string(*dict)) << "Element is incorrect";

    ASSERT_EQ(false, it1->next(&t)) << "Iterator has a no next value";

}

TEST_F(ControllerTest, GetDeltaMaterializedSnapshotPatch) {
    controller->new_patch_bulk()
            ->addition(hdt::TripleString("<a>", "<a>", "<a>"))
            ->addition(hdt::TripleString("<a>", "<a>", "<b>"))
            ->addition(hdt::TripleString("<a>", "<a>", "<c>"))
            ->commit();

    controller->new_patch_bulk()
            ->deletion(hdt::TripleString("<a>", "<a>", "<b>"))
            ->commit();

    controller->new_patch_bulk()
            ->addition(hdt::TripleString("<a>", "<a>", "<d>"))
            ->commit();

    std::shared_ptr<DictionaryManager> dict = controller->get_snapshot_manager()->get_dictionary_manager(0);

    // Expected version 0:
    // <a> <a> <a>
    // <a> <a> <b>
    // <a> <a> <c>

    // Expected version 1:
    // <a> <a> <a>
    // <a> <a> <c>

    // Expected version 2:
    // <a> <a> <a>
    // <a> <a> <c>
    // <a> <a> <d>

    TripleDelta t;

    // Request between versions 0 and 1 for ? ? ?
    ASSERT_EQ(1, controller->get_delta_materialized_count(Triple("", "", "", dict), 0, 1).first) << "Count is incorrect";
    TripleDeltaIterator* it0 = controller->get_delta_materialized(Triple("", "", "", dict), 0, 0, 1);

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <b>.", t.get_triple()->to_string(*dict)) << "Element is incorrect";
    ASSERT_EQ(false, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(false, it0->next(&t)) << "Iterator should be finished";

    // Request between versions 0 and 1 for <a> ? ?
    ASSERT_EQ(1, controller->get_delta_materialized_count(Triple("<a>", "", "", dict), 0, 1).first) << "Count is incorrect";
    TripleDeltaIterator* it1 = controller->get_delta_materialized(Triple("<a>", "", "", dict), 0, 0, 1);

    ASSERT_EQ(true, it1->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <b>.", t.get_triple()->to_string(*dict)) << "Element is incorrect";
    ASSERT_EQ(false, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(false, it1->next(&t)) << "Iterator should be finished";

    // Request between versions 0 and 1 for ? ? <d>
    ASSERT_EQ(0, controller->get_delta_materialized_count(Triple("", "", "<d>", dict), 0, 1).first) << "Count is incorrect";
    TripleDeltaIterator* it2 = controller->get_delta_materialized(Triple("", "", "<d>", dict), 0, 0, 1);

    ASSERT_EQ(false, it2->next(&t)) << "Iterator should be finished";

    // Request between versions 0 and 2 for <a> ? ?
    ASSERT_EQ(2, controller->get_delta_materialized_count(Triple("<a>", "", "", dict), 0, 2).first) << "Count is incorrect";
    TripleDeltaIterator* it3 = controller->get_delta_materialized(Triple("<a>", "", "", dict), 0, 0, 2);

    ASSERT_EQ(true, it3->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <b>.", t.get_triple()->to_string(*dict)) << "Element is incorrect";
    ASSERT_EQ(false, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(true, it3->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <d>.", t.get_triple()->to_string(*dict)) << "Element is incorrect";
    ASSERT_EQ(true, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(false, it3->next(&t)) << "Iterator should be finished";

    // Request between versions 0 and 2 for <a> ? ? with offset 1
    TripleDeltaIterator* it4 = controller->get_delta_materialized(Triple("<a>", "", "", dict), 1, 0, 2);

    ASSERT_EQ(true, it4->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <d>.", t.get_triple()->to_string(*dict)) << "Element is incorrect";
    ASSERT_EQ(true, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(false, it4->next(&t)) << "Iterator should be finished";

    // Request between versions 0 and 2 for <a> ? ? with offset 2
    TripleDeltaIterator* it5 = controller->get_delta_materialized(Triple("<a>", "", "", dict), 2, 0, 2);

    ASSERT_EQ(false, it5->next(&t)) << "Iterator should be finished";
}

TEST_F(ControllerMSTest, GetDeltaMaterializedSnapshotMS) {
    controller->new_patch_bulk()
            ->addition(hdt::TripleString("<a>", "<a>", "<a>"))
            ->addition(hdt::TripleString("<a>", "<a>", "<b>"))
            ->addition(hdt::TripleString("<a>", "<a>", "<c>"))
            ->commit();

    controller->new_patch_bulk()
            ->deletion(hdt::TripleString("<a>", "<a>", "<b>"))
            ->commit();

    controller->new_patch_bulk()
            ->addition(hdt::TripleString("<a>", "<a>", "<d>"))
            ->commit();

    controller->new_patch_bulk()
            ->addition(hdt::TripleString("<a>", "<a>", "<e>"))
            ->addition(hdt::TripleString("<a>", "<a>", "<f>"))
            ->commit();

    controller->new_patch_bulk()
            ->deletion(hdt::TripleString("<a>", "<a>", "<a>"))
            ->commit();

    controller->new_patch_bulk()
            ->deletion(hdt::TripleString("<a>", "<a>", "<e>"))
            ->commit();

    // Expected version 0:
    // <a> <a> <a>
    // <a> <a> <b>
    // <a> <a> <c>

    // Expected version 1:
    // <a> <a> <a>
    // <a> <a> <c>

    // Expected version 2:
    // <a> <a> <a>
    // <a> <a> <c>
    // <a> <a> <d>

    // Expected version 3:
    // <a> <a> <a>
    // <a> <a> <c>
    // <a> <a> <d>
    // <a> <a> <e>
    // <a> <a> <f>

    // Expected version 4
    // <a> <a> <c>
    // <a> <a> <d>
    // <a> <a> <e>
    // <a> <a> <f>

    // Expected version 5
    // <a> <a> <c>
    // <a> <a> <d>
    // <a> <a> <f>

    TripleDelta t;

    // Case snapshot-patch single delta chain
    // Request between versions 0 and 1 for ? ? ?
    ASSERT_EQ(1, controller->get_delta_materialized_count(StringTriple("", "", ""), 0, 1).first) << "Count is incorrect";
    TripleDeltaIterator* it0 = controller->get_delta_materialized(StringTriple("", "", ""), 0, 0, 1);

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <b>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(false, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(false, it0->next(&t)) << "Iterator should be finished";

    // Request between versions 0 and 1 for <a> ? ?
    ASSERT_EQ(1, controller->get_delta_materialized_count(StringTriple("<a>", "", ""), 0, 1).first) << "Count is incorrect";
    TripleDeltaIterator* it1 = controller->get_delta_materialized(StringTriple("<a>", "", ""), 0, 0, 1);

    ASSERT_EQ(true, it1->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <b>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(false, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(false, it1->next(&t)) << "Iterator should be finished";

    // Request between versions 0 and 1 for ? ? <d>
    ASSERT_EQ(0, controller->get_delta_materialized_count(StringTriple("", "", "<d>"), 0, 1).first) << "Count is incorrect";
    TripleDeltaIterator* it2 = controller->get_delta_materialized(StringTriple("", "", "<d>"), 0, 0, 1);

    ASSERT_EQ(false, it2->next(&t)) << "Iterator should be finished";

    // Request between versions 0 and 2 for <a> ? ?
    // Case snapshot-snapshot multiple delta chains
    ASSERT_EQ(2, controller->get_delta_materialized_count(StringTriple("<a>", "", ""), 0, 2).first) << "Count is incorrect";
    TripleDeltaIterator* it3 = controller->get_delta_materialized(StringTriple("<a>", "", ""), 0, 0, 2);

    ASSERT_EQ(true, it3->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <b>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(false, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(true, it3->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <d>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(true, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(false, it3->next(&t)) << "Iterator should be finished";

    // Request between versions 0 and 3 for ? ? ?
    TripleDeltaIterator* it4 = controller->get_delta_materialized(StringTriple("", "", ""), 0, 0, 3);

    ASSERT_EQ(true, it4->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <b>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(false, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(true, it4->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <d>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(true, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(true, it4->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <e>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(true, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(true, it4->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <f>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(true, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(false, it4->next(&t)) << "Iterator should be finished";

}

TEST_F(ControllerMSTest, GetDeltaMaterializedPatchMS) {
    controller->new_patch_bulk()
            ->addition(hdt::TripleString("<a>", "<a>", "<a>"))
            ->addition(hdt::TripleString("<a>", "<a>", "<b>"))
            ->addition(hdt::TripleString("<a>", "<a>", "<c>"))
            ->commit();

    controller->new_patch_bulk()
            ->deletion(hdt::TripleString("<a>", "<a>", "<b>"))
            ->addition(hdt::TripleString("<a>", "<b>", "<c>"))
            ->commit();

    controller->new_patch_bulk()
            ->deletion(hdt::TripleString("<a>", "<b>", "<c>"))
            ->addition(hdt::TripleString("<a>", "<a>", "<d>"))
            ->commit();

    controller->new_patch_bulk()
            ->addition(hdt::TripleString("<a>", "<a>", "<e>"))
            ->addition(hdt::TripleString("<a>", "<a>", "<f>"))
            ->commit();

    controller->new_patch_bulk()
            ->deletion(hdt::TripleString("<a>", "<a>", "<a>"))
            ->commit();

    controller->new_patch_bulk()
            ->deletion(hdt::TripleString("<a>", "<a>", "<e>"))
            ->commit();

    // Expected version 0:
    // <a> <a> <a>
    // <a> <a> <b>
    // <a> <a> <c>

    // Expected version 1:
    // <a> <a> <a>
    // <a> <a> <c>
    // <a> <b> <c>

    // Expected version 2:
    // <a> <a> <a>
    // <a> <a> <c>
    // <a> <a> <d>

    // Expected version 3:
    // <a> <a> <a>
    // <a> <a> <c>
    // <a> <a> <d>
    // <a> <a> <e>
    // <a> <a> <f>

    // Expected version 4
    // <a> <a> <c>
    // <a> <a> <d>
    // <a> <a> <e>
    // <a> <a> <f>

    // Expected version 5
    // <a> <a> <c>
    // <a> <a> <d>
    // <a> <a> <f>

    TripleDelta t;

    // Case patch-patch two delta chain
    // Request between versions 1 and 3 for ? ? ?
    ASSERT_EQ(4, controller->get_delta_materialized_count(StringTriple("", "", ""), 1, 3).first) << "Count is incorrect";
    TripleDeltaIterator* it0 = controller->get_delta_materialized(StringTriple("", "", ""), 0, 1, 3);

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <d>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(true, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <e>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(true, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <f>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(true, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <b> <c>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(false, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(false, it0->next(&t)) << "Iterator should be finished";


    ASSERT_EQ(5, controller->get_delta_materialized_count(StringTriple("", "", ""), 1, 4).first) << "Count is incorrect";
    TripleDeltaIterator* it1 = controller->get_delta_materialized(StringTriple("", "", ""), 0, 1, 4);

    ASSERT_EQ(true, it1->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <a>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(false, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(true, it1->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <d>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(true, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(true, it1->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <e>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(true, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(true, it1->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <f>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(true, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(true, it1->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <b> <c>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(false, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(false, it1->next(&t)) << "Iterator should be finished";

}

TEST_F(ControllerMSTest, GetDeltaMaterializedOrderMS) {
    controller->new_patch_bulk()
            ->addition(hdt::TripleString("<a>", "<b>", "<a>"))
            ->addition(hdt::TripleString("<a>", "<b>", "<b>"))
            ->addition(hdt::TripleString("<a>", "<b>", "<c>"))
            ->commit();

    controller->new_patch_bulk()
            ->deletion(hdt::TripleString("<a>", "<b>", "<b>"))
            ->addition(hdt::TripleString("<a>", "<b>", "<d>"))
            ->commit();

    controller->new_patch_bulk()
            ->deletion(hdt::TripleString("<a>", "<b>", "<c>"))
            ->addition(hdt::TripleString("<a>", "<b>", "<e>"))
            ->commit();

    controller->new_patch_bulk()
            ->addition(hdt::TripleString("<a>", "<b>", "<g>"))
            ->addition(hdt::TripleString("<a>", "<b>", "<h>"))
            ->commit();

    controller->new_patch_bulk()
            ->addition(hdt::TripleString("<n>", "<b>", "<a>"))
            ->commit();

    controller->new_patch_bulk()
            ->addition(hdt::TripleString("<n>", "<b>", "<c>"))
            ->addition(hdt::TripleString("<b>", "<b>", "<b>"))
            ->commit();


    // Expected version 0:
    // <a> <b> <a>
    // <a> <b> <b>
    // <a> <b> <c>

    // Expected version 1:
    // <a> <b> <a>
    // <a> <b> <c>
    // <a> <b> <d>

    // Expected version 2:
    // <a> <b> <a>
    // <a> <b> <d>
    // <a> <b> <e>

    // Expected version 3:
    // <a> <b> <a>
    // <a> <b> <d>
    // <a> <b> <e>
    // <a> <b> <g>
    // <a> <b> <h>

    // Expected version 4:
    // <a> <b> <a>
    // <a> <b> <d>
    // <a> <b> <e>
    // <a> <b> <g>
    // <a> <b> <h>
    // <n> <b> <a>

    // Expected version 5:
    // <a> <b> <a>
    // <a> <b> <d>
    // <a> <b> <e>
    // <a> <b> <g>
    // <a> <b> <h>
    // <b> <b> <b>
    // <n> <b> <a>
    // <n> <b> <c>


    TripleDelta t;

    // Case patch-patch two delta chain
    // Request between versions 1 and 3 for ? ? ?
    ASSERT_EQ(2, controller->get_delta_materialized_count(StringTriple("", "<b>", ""), 1, 2).first) << "Count is incorrect";
    TripleDeltaIterator* it0 = controller->get_delta_materialized(StringTriple("", "<b>", ""), 0, 1, 2);

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <b> <c>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(false, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <b> <e>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(true, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(false, it0->next(&t)) << "Iterator should be finished";


    ASSERT_EQ(4, controller->get_delta_materialized_count(StringTriple("", "<b>", ""), 1, 3).first) << "Count is incorrect";
    TripleDeltaIterator* it1 = controller->get_delta_materialized(StringTriple("", "<b>", ""), 0, 1, 3);

    ASSERT_EQ(true, it1->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <b> <c>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(false, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(true, it1->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <b> <e>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(true, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(true, it1->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <b> <g>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(true, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(true, it1->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <b> <h>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(true, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(false, it1->next(&t)) << "Iterator should be finished";


    ASSERT_EQ(6, controller->get_delta_materialized_count(StringTriple("", "<b>", ""), 0, 3).first) << "Count is incorrect";
    TripleDeltaIterator* it2 = controller->get_delta_materialized(StringTriple("", "<b>", ""), 0, 0, 3);

    ASSERT_EQ(true, it2->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <b> <b>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(false, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(true, it2->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <b> <c>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(false, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(true, it2->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <b> <d>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(true, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(true, it2->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <b> <e>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(true, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(true, it2->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <b> <g>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(true, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(true, it2->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <b> <h>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(true, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(false, it2->next(&t)) << "Iterator should be finished";


    ASSERT_EQ(9, controller->get_delta_materialized_count(StringTriple("", "<b>", ""), 0, 5).first) << "Count is incorrect";
    TripleDeltaIterator* it3 = controller->get_delta_materialized(StringTriple("", "<b>", ""), 0, 0, 5);

    ASSERT_EQ(true, it3->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <b> <b>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(false, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(true, it3->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <b> <c>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(false, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(true, it3->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <b> <d>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(true, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(true, it3->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <b> <e>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(true, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(true, it3->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <b> <g>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(true, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(true, it3->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <b> <h>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(true, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(true, it3->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<b> <b> <b>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(true, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(true, it3->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<n> <b> <a>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(true, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(true, it3->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<n> <b> <c>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(true, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(false, it3->next(&t)) << "Iterator should be finished";


    TripleDeltaIterator* it4 = controller->get_delta_materialized(StringTriple("", "<b>", ""), 0, 4, 5);
    ASSERT_EQ(2, controller->get_delta_materialized_count(StringTriple("", "<b>", ""), 4, 5).first) << "Count is incorrect";

    ASSERT_EQ(true, it4->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<b> <b> <b>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(true, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(true, it4->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<n> <b> <c>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(true, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(false, it4->next(&t)) << "Iterator should be finished";
}


TEST_F(ControllerTest, GetDeltaMaterializedPatchPatch) {
    controller->new_patch_bulk()
            ->addition(hdt::TripleString("<a>", "<a>", "<a>"))
            ->addition(hdt::TripleString("<a>", "<a>", "<b>"))
            ->addition(hdt::TripleString("<a>", "<a>", "<c>"))
            ->commit();

    controller->new_patch_bulk()
            ->deletion(hdt::TripleString("<a>", "<a>", "<b>"))
            ->commit();

    controller->new_patch_bulk()
            ->addition(hdt::TripleString("<a>", "<a>", "<d>"))
            ->commit();

    controller->new_patch_bulk()
            ->addition(hdt::TripleString("<a>", "<a>", "<e>"))
            ->deletion(hdt::TripleString("<a>", "<a>", "<a>"))
            ->commit();

    std::shared_ptr<DictionaryManager> dict = controller->get_snapshot_manager()->get_dictionary_manager(0);

    // Expected version 0:
    // <a> <a> <a>
    // <a> <a> <b>
    // <a> <a> <c>

    // Expected version 1:
    // <a> <a> <a>
    // <a> <a> <c>

    // Expected version 2:
    // <a> <a> <a>
    // <a> <a> <c>
    // <a> <a> <d>

    // Expected version 3:
    // <a> <a> <c>
    // <a> <a> <d>
    // <a> <a> <e>

    TripleDelta t;

    // Request between versions 1 and 2 for ? ? ?
    ASSERT_EQ(3, controller->get_delta_materialized_count(Triple("", "", "", dict), 1, 2, true).first) << "Count is incorrect";
    ASSERT_EQ(1, controller->get_delta_materialized_count(Triple("", "", "", dict), 1, 2, false).first) << "Count is incorrect";
    TripleDeltaIterator* it0 = controller->get_delta_materialized(Triple("", "", "", dict), 0, 1, 2);

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <d>.", t.get_triple()->to_string(*dict)) << "Element is incorrect";
    ASSERT_EQ(true, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(false, it0->next(&t)) << "Iterator should be finished";

    // Request between versions 2 and 3 for ? ? ?
    ASSERT_EQ(2, controller->get_delta_materialized_count(Triple("", "", "", dict), 2, 3).first) << "Count is incorrect";
    TripleDeltaIterator* it1 = controller->get_delta_materialized(Triple("", "", "", dict), 0, 2, 3);

    ASSERT_EQ(true, it1->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <a>.", t.get_triple()->to_string(*dict)) << "Element is incorrect";
    ASSERT_EQ(false, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(true, it1->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <e>.", t.get_triple()->to_string(*dict)) << "Element is incorrect";
    ASSERT_EQ(true, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(false, it1->next(&t)) << "Iterator should be finished";

    // Request between versions 2 and 3 for ? ? ? with offset 1
    TripleDeltaIterator* it1_b = controller->get_delta_materialized(Triple("", "", "", dict), 1, 2, 3);

    ASSERT_EQ(true, it1_b->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <e>.", t.get_triple()->to_string(*dict)) << "Element is incorrect";
    ASSERT_EQ(true, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(false, it1_b->next(&t)) << "Iterator should be finished";

    // Request between versions 2 and 3 for ? ? ? with offset 2
    TripleDeltaIterator* it1_c = controller->get_delta_materialized(Triple("", "", "", dict), 2, 2, 3);

    ASSERT_EQ(false, it1_c->next(&t)) << "Iterator should be finished";

    // Request between versions 1 and 3 for ? ? ?
    ASSERT_EQ(3, controller->get_delta_materialized_count(Triple("", "", "", dict), 1, 3).first) << "Count is incorrect";
    TripleDeltaIterator* it2 = controller->get_delta_materialized(Triple("", "", "", dict), 0, 1, 3);

    ASSERT_EQ(true, it2->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <a>.", t.get_triple()->to_string(*dict)) << "Element is incorrect";
    ASSERT_EQ(false, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(true, it2->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <d>.", t.get_triple()->to_string(*dict)) << "Element is incorrect";
    ASSERT_EQ(true, t.is_addition()) << "Element is incorrect";
    
    ASSERT_EQ(true, it2->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <e>.", t.get_triple()->to_string(*dict)) << "Element is incorrect";
    ASSERT_EQ(true, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(false, it2->next(&t)) << "Iterator should be finished";

    // Request between versions 1 and 3 for ? ? ? with offset 1
    TripleDeltaIterator* it2_b = controller->get_delta_materialized(Triple("", "", "", dict), 1, 1, 3);

    ASSERT_EQ(true, it2_b->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <d>.", t.get_triple()->to_string(*dict)) << "Element is incorrect";
    ASSERT_EQ(true, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(true, it2_b->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <e>.", t.get_triple()->to_string(*dict)) << "Element is incorrect";
    ASSERT_EQ(true, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(false, it2_b->next(&t)) << "Iterator should be finished";

    // Request between versions 1 and 3 for ? ? ? with offset 2
    TripleDeltaIterator* it2_c = controller->get_delta_materialized(Triple("", "", "", dict), 2, 1, 3);

    ASSERT_EQ(true, it2_c->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <e>.", t.get_triple()->to_string(*dict)) << "Element is incorrect";
    ASSERT_EQ(true, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(false, it2_c->next(&t)) << "Iterator should be finished";

    // Request between versions 1 and 3 for ? ? ? with offset 3
    TripleDeltaIterator* it2_d = controller->get_delta_materialized(Triple("", "", "", dict), 3, 1, 3);

    ASSERT_EQ(false, it2_d->next(&t)) << "Iterator should be finished";
}

TEST_F(ControllerMSTest, EdgeCaseGetDeltaMaterializedMS) {
    controller->new_patch_bulk()
            ->addition(hdt::TripleString("<a>", "<a>", "\"a\"^^<http://example.org/literal>"))
            ->addition(hdt::TripleString("<a>", "<a>", "\"b\"^^<http://example.org/literal>"))
            ->addition(hdt::TripleString("<a>", "<b>", "<a>"))
            ->addition(hdt::TripleString("<a>", "<b>", "<c>"))
            ->addition(hdt::TripleString("<a>", "<b>", "<d>"))
            ->addition(hdt::TripleString("<a>", "<b>", "<f>"))
            ->addition(hdt::TripleString("<a>", "<b>", "<z>"))
            ->addition(hdt::TripleString("<c>", "<c>", "<c>"))
            ->commit();

    controller->new_patch_bulk()
            ->deletion(hdt::TripleString("<a>", "<a>", "\"b\"^^<http://example.org/literal>"))
            ->addition(hdt::TripleString("<a>", "<a>", "\"z\"^^<http://example.org/literal>"))
            ->deletion(hdt::TripleString("<a>", "<b>", "<a>"))
            ->addition(hdt::TripleString("<a>", "<b>", "<g>"))
            ->deletion(hdt::TripleString("<a>", "<b>", "<z>"))
            ->addition(hdt::TripleString("<f>", "<f>", "<f>"))
            ->addition(hdt::TripleString("<z>", "<z>", "<z>"))
            ->commit();

    controller->new_patch_bulk()
            ->deletion(hdt::TripleString("<a>", "<a>", "\"z\"^^<http://example.org/literal>"))
            ->deletion(hdt::TripleString("<f>", "<f>", "<f>"))
            ->addition(hdt::TripleString("<f>", "<r>", "<s>"))
            ->addition(hdt::TripleString("<q>", "<q>", "<q>"))
            ->addition(hdt::TripleString("<r>", "<r>", "<r>"))
            ->commit();

    controller->new_patch_bulk()
            ->deletion(hdt::TripleString("<z>", "<z>", "<z>"))
            ->addition(hdt::TripleString("<z>", "<z>", "\"z\"^^<http://example.org/literal>"))
            ->commit();

    TripleDelta t;

    ASSERT_EQ(7, controller->get_delta_materialized_count(StringTriple("", "", ""), 1, 3, false).first) << "Count is incorrect";
    TripleDeltaIterator* it0 = controller->get_delta_materialized(StringTriple("", "", ""), 0, 1, 3);

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> \"z\"^^<http://example.org/literal>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(false, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<f> <f> <f>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(false, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<f> <r> <s>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(true, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<q> <q> <q>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(true, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<r> <r> <r>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(true, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<z> <z> \"z\"^^<http://example.org/literal>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(true, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<z> <z> <z>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(false, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(false, it0->next(&t)) << "Iterator should be finished";

    // TODO: test between 0 and 3
    // TODO: test between 0 and 2
}


TEST_F(ControllerMSTest2, EdgeCaseGetDeltaMaterializedMS2) {
    // Snapshot 0
    // <a> <a> <a>
    // <a> <a> <b>
    controller->new_patch_bulk()
            ->addition(hdt::TripleString("<a>", "<a>", "<a>"))
            ->addition(hdt::TripleString("<a>", "<a>", "<b>"))
            ->commit();

    // Patch 1
    // <a> <a> <a>
    // <a> <a> <b>
    // <a> <a> <c>
    controller->new_patch_bulk()
            ->addition(hdt::TripleString("<a>", "<a>", "<c>"))
            ->commit();

    // Patch 2
    // <a> <a> <a>
    // <a> <a> <b>
    // <a> <a> <c>
    // <a> <a> <d>
    controller->new_patch_bulk()
            ->addition(hdt::TripleString("<a>", "<a>", "<d>"))
            ->commit();

    // Snapshot 3
    // <a> <a> <a>
    // <a> <a> <c>
    // <a> <a> <d>
    // <a> <a> <e>
    controller->new_patch_bulk()
            ->deletion(hdt::TripleString("<a>", "<a>", "<b>"))
            ->addition(hdt::TripleString("<a>", "<a>", "<e>"))
            ->commit();

    // Patch 4
    // <a> <a> <a>
    // <a> <a> <b>
    // <a> <a> <c>
    // <a> <a> <d>
    controller->new_patch_bulk()
            ->addition(hdt::TripleString("<a>", "<a>", "<b>"))
            ->deletion(hdt::TripleString("<a>", "<a>", "<e>"))
            ->commit();

    // Patch 5
    // <a> <a> <a>
    // <a> <a> <c>
    // <a> <a> <d>
    // <a> <a> <e>
    controller->new_patch_bulk()
            ->deletion(hdt::TripleString("<a>", "<a>", "<b>"))
            ->addition(hdt::TripleString("<a>", "<a>", "<e>"))
            ->commit();

    TripleDelta t;

    // Expected delta 0 -> 5
    // - <a> <a> <b>
    // + <a> <a> <c>
    // + <a> <a> <d>
    // + <a> <a> <e>
    ASSERT_EQ(4, controller->get_delta_materialized_count(StringTriple("", "", ""), 0, 5, false).first) << "Count is incorrect";
    TripleDeltaIterator* it0 = controller->get_delta_materialized(StringTriple("", "", ""), 0, 0, 5);

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <b>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(false, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <c>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(true, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <d>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(true, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <e>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(true, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(false, it0->next(&t)) << "Iterator should be finished";

    // Expected delta 1 -> 5
    // - <a> <a> <b>
    // + <a> <a> <d>
    // + <a> <a> <e>
    ASSERT_EQ(3, controller->get_delta_materialized_count(StringTriple("", "", ""), 1, 5, false).first) << "Count is incorrect";
    TripleDeltaIterator* it1 = controller->get_delta_materialized(StringTriple("", "", ""), 0, 1, 5);

    ASSERT_EQ(true, it1->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <b>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(false, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(true, it1->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <d>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(true, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(true, it1->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <e>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(true, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(false, it1->next(&t)) << "Iterator should be finished";
}


TEST_F(ControllerTest, EdgeCaseGetDeltaMaterialized1) {
    /*
     * Test if DM between two patches is correct.
     * We specifically test the case for a triple that is added in 1, and removed again in 3.
     * We check if DM 1-2 correctly emits the addition, and if DM 3-4 correctly emits the deletion.
     */
    controller->new_patch_bulk()
            ->addition(hdt::TripleString("<a>", "<a>", "<a>"))
            ->commit();

    controller->new_patch_bulk()
            ->addition(hdt::TripleString("<z>", "<z>", "<z>"))
            ->commit();

    controller->new_patch_bulk()
            ->addition(hdt::TripleString("<a>", "<a>", "<b>"))
            ->commit();

    controller->new_patch_bulk()
            ->deletion(hdt::TripleString("<a>", "<a>", "<a>"))
            ->commit();

    controller->new_patch_bulk()
            ->deletion(hdt::TripleString("<a>", "<a>", "<b>"))
            ->commit();

    controller->new_patch_bulk()
            ->addition(hdt::TripleString("<y>", "<y>", "<y>"))
            ->commit();

    std::shared_ptr<DictionaryManager> dict = controller->get_snapshot_manager()->get_dictionary_manager(0);

    // Expected version 0:
    // <a> <a> <a>

    // Expected version 1:
    // <a> <a> <a>
    // <z> <z> <z>

    // Expected version 2:
    // <a> <a> <a>
    // <a> <a> <b>
    // <z> <z> <z>

    // Expected version 3:
    // <a> <a> <b>
    // <z> <z> <z>

    // Expected version 4:
    // <z> <z> <z>

    // Expected version 5:
    // <y> <y> <y>
    // <z> <z> <z>

    TripleDelta t;

    // Request between versions 1 and 2 for ? ? ?
    TripleDeltaIterator* it0 = controller->get_delta_materialized(Triple("", "", "", dict), 0, 1, 2);

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <b>.", t.get_triple()->to_string(*dict)) << "Element is incorrect";
    ASSERT_EQ(true, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(false, it0->next(&t)) << "Iterator should be finished";

    // Request between versions 3 and 4 for ? ? ?
    TripleDeltaIterator* it1 = controller->get_delta_materialized(Triple("", "", "", dict), 0, 3, 4);

    ASSERT_EQ(true, it1->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <b>.", t.get_triple()->to_string(*dict)) << "Element is incorrect";
    ASSERT_EQ(false, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(false, it1->next(&t)) << "Iterator should be finished";
}

TEST_F(ControllerTest, EdgeCaseGetDeltaMaterialized2) {
    /*
     * Test if DM is correct if we repeatedly add and remove the same triple.
     */
    controller->new_patch_bulk()
            ->addition(hdt::TripleString("<a>", "<a>", "<a>"))
            ->commit();

    controller->new_patch_bulk()
            ->deletion(hdt::TripleString("<a>", "<a>", "<a>"))
            ->commit();

    controller->new_patch_bulk()
            ->addition(hdt::TripleString("<a>", "<a>", "<a>"))
            ->commit();

    controller->new_patch_bulk()
            ->deletion(hdt::TripleString("<a>", "<a>", "<a>"))
            ->commit();

    controller->new_patch_bulk()
            ->addition(hdt::TripleString("<a>", "<a>", "<a>"))
            ->commit();

    std::shared_ptr<DictionaryManager> dict = controller->get_snapshot_manager()->get_dictionary_manager(0);

    // Expected version 0:
    // <a> <a> <a>

    // Expected version 1:

    // Expected version 2:
    // <a> <a> <a>

    // Expected version 3:

    // Expected version 4:
    // <a> <a> <a>

    TripleDelta t;

    // Request between versions 0 and 1 for ? ? ?
    TripleDeltaIterator* it0 = controller->get_delta_materialized(Triple("", "", "", dict), 0, 0, 1);

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <a>.", t.get_triple()->to_string(*dict)) << "Element is incorrect";
    ASSERT_EQ(false, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(false, it0->next(&t)) << "Iterator should be finished";

    // Request between versions 1 and 2 for ? ? ?
    TripleDeltaIterator* it1 = controller->get_delta_materialized(Triple("", "", "", dict), 0, 1, 2);

    ASSERT_EQ(true, it1->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <a>.", t.get_triple()->to_string(*dict)) << "Element is incorrect";
    ASSERT_EQ(true, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(false, it1->next(&t)) << "Iterator should be finished";

    // Request between versions 2 and 3 for ? ? ?
    TripleDeltaIterator* it2 = controller->get_delta_materialized(Triple("", "", "", dict), 0, 2, 3);

    ASSERT_EQ(true, it2->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <a>.", t.get_triple()->to_string(*dict)) << "Element is incorrect";
    ASSERT_EQ(false, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(false, it2->next(&t)) << "Iterator should be finished";

    // Request between versions 3 and 4 for ? ? ?
    TripleDeltaIterator* it3 = controller->get_delta_materialized(Triple("", "", "", dict), 0, 3, 4);

    ASSERT_EQ(true, it3->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <a>.", t.get_triple()->to_string(*dict)) << "Element is incorrect";
    ASSERT_EQ(true, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(false, it3->next(&t)) << "Iterator should be finished";
}

TEST_F(ControllerTest, EdgeCaseGetDeltaMaterialized3) {
    /*
     * Test if DM is correct when the range does not match the changeset bounds,
     * and the triple *is not* present in the snapshot.
     */
    controller->new_patch_bulk()
            ->addition(hdt::TripleString("<dummy>", "<dummy>", "<dummy>"))
            ->commit();

    controller->new_patch_bulk()
            ->addition(hdt::TripleString("<dummy2>", "<dummy2>", "<dummy2>"))
            ->commit();

    controller->new_patch_bulk()
            ->addition(hdt::TripleString("<a>", "<a>", "<a>"))
            ->commit();

    controller->new_patch_bulk()
            ->addition(hdt::TripleString("<dummy3>", "<dummy3>", "<dummy3>"))
            ->commit();

    controller->new_patch_bulk()
            ->deletion(hdt::TripleString("<a>", "<a>", "<a>"))
            ->commit();

    std::shared_ptr<DictionaryManager> dict = controller->get_snapshot_manager()->get_dictionary_manager(0);

    // Expected version 0:

    // Expected version 1:

    // Expected version 2:
    // <a> <a> <a>

    // Expected version 3:
    // <a> <a> <a>

    // Expected version 4:

    TripleDelta t;

    // Request between versions 1 and 4 for <a> ? ?
    TripleDeltaIterator* it0 = controller->get_delta_materialized(Triple("<a>", "", "", dict), 0, 1, 4);
    ASSERT_EQ(false, it0->next(&t)) << "Iterator should be finished";

    // Request between versions 2 and 4 for <a> ? ?
    TripleDeltaIterator* it1 = controller->get_delta_materialized(Triple("<a>", "", "", dict), 0, 2, 4);

    ASSERT_EQ(true, it1->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <a>.", t.get_triple()->to_string(*dict)) << "Element is incorrect";
    ASSERT_EQ(false, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(false, it1->next(&t)) << "Iterator should be finished";

    // Request between versions 1 and 3 for <a> ? ?
    TripleDeltaIterator* it2 = controller->get_delta_materialized(Triple("<a>", "", "", dict), 0, 1, 3);

    ASSERT_EQ(true, it2->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <a>.", t.get_triple()->to_string(*dict)) << "Element is incorrect";
    ASSERT_EQ(true, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(false, it2->next(&t)) << "Iterator should be finished";
}

TEST_F(ControllerTest, EdgeCaseGetDeltaMaterialized4) {
    /*
     * Test if DM is correct when the range does not match the changeset bounds,
     * and the triple *is* present in the snapshot.
     */
    controller->new_patch_bulk()
            ->addition(hdt::TripleString("<a>", "<a>", "<a>"))
            ->commit();

    controller->new_patch_bulk()
            ->addition(hdt::TripleString("<dummy>", "<dummy>", "<dummy>"))
            ->commit();

    controller->new_patch_bulk()
            ->deletion(hdt::TripleString("<a>", "<a>", "<a>"))
            ->commit();

    controller->new_patch_bulk()
            ->addition(hdt::TripleString("<dummy2>", "<dummy2>", "<dummy2>"))
            ->commit();

    controller->new_patch_bulk()
            ->addition(hdt::TripleString("<a>", "<a>", "<a>"))
            ->commit();

    std::shared_ptr<DictionaryManager> dict = controller->get_snapshot_manager()->get_dictionary_manager(0);

    // Expected version 0:
    // <a> <a> <a>

    // Expected version 1:
    // <a> <a> <a>

    // Expected version 2:

    // Expected version 3:

    // Expected version 4:
    // <a> <a> <a>

    TripleDelta t;

    // Request between versions 1 and 4 for <a> ? ?
    TripleDeltaIterator* it0 = controller->get_delta_materialized(Triple("<a>", "", "", dict), 0, 1, 4);
    ASSERT_EQ(false, it0->next(&t)) << "Iterator should be finished";

    // Request between versions 2 and 4 for <a> ? ?
    TripleDeltaIterator* it1 = controller->get_delta_materialized(Triple("<a>", "", "", dict), 0, 2, 4);

    ASSERT_EQ(true, it1->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <a>.", t.get_triple()->to_string(*dict)) << "Element is incorrect";
    ASSERT_EQ(true, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(false, it1->next(&t)) << "Iterator should be finished";

    // Request between versions 1 and 3 for <a> ? ?
    TripleDeltaIterator* it2 = controller->get_delta_materialized(Triple("<a>", "", "", dict), 0, 1, 3);

    ASSERT_EQ(true, it2->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <a>.", t.get_triple()->to_string(*dict)) << "Element is incorrect";
    ASSERT_EQ(false, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(false, it2->next(&t)) << "Iterator should be finished";
}

TEST_F(ControllerTest, EdgeCaseGetDeltaMaterialized5) {
    /*
     * Test if DM relative to snapshot is correct when the range does not match the changeset bounds,
     * and the triple *is not* present in the snapshot.
     */
    controller->new_patch_bulk()
            ->addition(hdt::TripleString("<dummy>", "<dummy>", "<dummy>"))
            ->commit();

    controller->new_patch_bulk()
            ->addition(hdt::TripleString("<a>", "<a>", "<a>"))
            ->commit();

    controller->new_patch_bulk()
            ->deletion(hdt::TripleString("<a>", "<a>", "<a>"))
            ->commit();

    std::shared_ptr<DictionaryManager> dict = controller->get_snapshot_manager()->get_dictionary_manager(0);

    // Expected version 0:

    // Expected version 1:
    // <a> <a> <a>

    // Expected version 2:

    TripleDelta t;

    // Request between versions 0 and 2 for <a> ? ?
    TripleDeltaIterator* it0 = controller->get_delta_materialized(Triple("<a>", "", "", dict), 0, 0, 2);
    ASSERT_EQ(false, it0->next(&t)) << "Iterator should be finished";

    // Request between versions 0 and 1 for <a> ? ?
    TripleDeltaIterator* it1 = controller->get_delta_materialized(Triple("<a>", "", "", dict), 0, 0, 1);

    ASSERT_EQ(true, it1->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <a>.", t.get_triple()->to_string(*dict)) << "Element is incorrect";
    ASSERT_EQ(true, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(false, it1->next(&t)) << "Iterator should be finished";

    // Request between versions 1 and 2 for <a> ? ?
    TripleDeltaIterator* it2 = controller->get_delta_materialized(Triple("<a>", "", "", dict), 0, 1, 2);

    ASSERT_EQ(true, it2->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <a>.", t.get_triple()->to_string(*dict)) << "Element is incorrect";
    ASSERT_EQ(false, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(false, it2->next(&t)) << "Iterator should be finished";
}

TEST_F(ControllerTest, EdgeCaseGetDeltaMaterialized6) {
    /*
     * Test if DM relative to snapshot is correct when the range does not match the changeset bounds,
     * and the triple *is* present in the snapshot.
     */
    controller->new_patch_bulk()
            ->addition(hdt::TripleString("<a>", "<a>", "<a>"))
            ->commit();

    controller->new_patch_bulk()
            ->deletion(hdt::TripleString("<a>", "<a>", "<a>"))
            ->commit();

    controller->new_patch_bulk()
            ->addition(hdt::TripleString("<a>", "<a>", "<a>"))
            ->commit();

    std::shared_ptr<DictionaryManager> dict = controller->get_snapshot_manager()->get_dictionary_manager(0);

    // Expected version 0:
    // <a> <a> <a>

    // Expected version 1:

    // Expected version 2:
    // <a> <a> <a>

    TripleDelta t;

    // Request between versions 0 and 2 for <a> ? ?
    TripleDeltaIterator* it0 = controller->get_delta_materialized(Triple("<a>", "", "", dict), 0, 0, 2);
    ASSERT_EQ(false, it0->next(&t)) << "Iterator should be finished";

    // Request between versions 0 and 1 for <a> ? ?
    TripleDeltaIterator* it1 = controller->get_delta_materialized(Triple("<a>", "", "", dict), 0, 0, 1);

    ASSERT_EQ(true, it1->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <a>.", t.get_triple()->to_string(*dict)) << "Element is incorrect";
    ASSERT_EQ(false, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(false, it1->next(&t)) << "Iterator should be finished";

    // Request between versions 1 and 2 for <a> ? ?
    TripleDeltaIterator* it2 = controller->get_delta_materialized(Triple("<a>", "", "", dict), 0, 1, 2);

    ASSERT_EQ(true, it2->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <a>.", t.get_triple()->to_string(*dict)) << "Element is incorrect";
    ASSERT_EQ(true, t.is_addition()) << "Element is incorrect";

    ASSERT_EQ(false, it2->next(&t)) << "Iterator should be finished";
}

TEST_F(ControllerTest, GetVersionSnapshot) {
    controller->new_patch_bulk()
            ->addition(hdt::TripleString("<a>", "<a>", "<a>"))
            ->addition(hdt::TripleString("<a>", "<a>", "<b>"))
            ->addition(hdt::TripleString("<a>", "<a>", "<c>"))
            ->commit();

    std::shared_ptr<DictionaryManager> dict = controller->get_snapshot_manager()->get_dictionary_manager(0);

    // Expected version 0:
    // <a> <a> <a>
    // <a> <a> <b>
    // <a> <a> <c>

    TripleVersions t;

    std::vector<int> v_0(0);
    v_0.push_back(0);

    // Request versions for ? ? ?
    ASSERT_EQ(3, controller->get_version_count(Triple("", "", "", dict)).first) << "Count is incorrect";
    TripleVersionsIterator* it0 = controller->get_version(Triple("", "", "", dict), 0);

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <a>.", t.get_triple()->to_string(*dict)) << "Element is incorrect";
    ASSERT_EQ(v_0, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <b>.", t.get_triple()->to_string(*dict)) << "Element is incorrect";
    ASSERT_EQ(v_0, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <c>.", t.get_triple()->to_string(*dict)) << "Element is incorrect";
    ASSERT_EQ(v_0, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(false, it0->next(&t)) << "Iterator should be finished";

    // Request versions for ? ? ? with offset 1
    TripleVersionsIterator* it1 = controller->get_version(Triple("", "", "", dict), 1);

    ASSERT_EQ(true, it1->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <b>.", t.get_triple()->to_string(*dict)) << "Element is incorrect";
    ASSERT_EQ(v_0, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(true, it1->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <c>.", t.get_triple()->to_string(*dict)) << "Element is incorrect";
    ASSERT_EQ(v_0, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(false, it1->next(&t)) << "Iterator should be finished";

    // Request versions for ? ? ? with offset 2
    TripleVersionsIterator* it2 = controller->get_version(Triple("", "", "", dict), 2);

    ASSERT_EQ(true, it2->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <c>.", t.get_triple()->to_string(*dict)) << "Element is incorrect";
    ASSERT_EQ(v_0, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(false, it2->next(&t)) << "Iterator should be finished";

    // Request versions for ? ? ? with offset 3
    TripleVersionsIterator* it3 = controller->get_version(Triple("", "", "", dict), 3);

    ASSERT_EQ(false, it3->next(&t)) << "Iterator should be finished";

    // Request versions for <a> ? ?
    ASSERT_EQ(3, controller->get_version_count(Triple("<a>", "", "", dict)).first) << "Count is incorrect";
    TripleVersionsIterator* it5 = controller->get_version(Triple("", "", "", dict), 0);

    ASSERT_EQ(true, it5->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <a>.", t.get_triple()->to_string(*dict)) << "Element is incorrect";
    ASSERT_EQ(v_0, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(true, it5->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <b>.", t.get_triple()->to_string(*dict)) << "Element is incorrect";
    ASSERT_EQ(v_0, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(true, it5->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <c>.", t.get_triple()->to_string(*dict)) << "Element is incorrect";
    ASSERT_EQ(v_0, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(false, it5->next(&t)) << "Iterator should be finished";

    // Request versions for <a> ? ? with offset 1
    TripleVersionsIterator* it6 = controller->get_version(Triple("<a>", "", "", dict), 1);

    ASSERT_EQ(true, it6->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <b>.", t.get_triple()->to_string(*dict)) << "Element is incorrect";
    ASSERT_EQ(v_0, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(true, it6->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <c>.", t.get_triple()->to_string(*dict)) << "Element is incorrect";
    ASSERT_EQ(v_0, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(false, it6->next(&t)) << "Iterator should be finished";

    // Request versions for <a> ? ? with offset 2
    TripleVersionsIterator* it7 = controller->get_version(Triple("<a>", "", "", dict), 2);

    ASSERT_EQ(true, it7->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <c>.", t.get_triple()->to_string(*dict)) << "Element is incorrect";
    ASSERT_EQ(v_0, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(false, it7->next(&t)) << "Iterator should be finished";

    // Request versions for <a> ? ? with offset 3
    TripleVersionsIterator* it8 = controller->get_version(Triple("<a>", "", "", dict), 3);

    ASSERT_EQ(false, it8->next(&t)) << "Iterator should be finished";

    // Request versions for ? ? <a>
    ASSERT_EQ(1, controller->get_version_count(Triple("", "", "<a>", dict)).first) << "Count is incorrect";
    TripleVersionsIterator* it10 = controller->get_version(Triple("", "", "<a>", dict), 0);

    ASSERT_EQ(true, it10->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <a>.", t.get_triple()->to_string(*dict)) << "Element is incorrect";
    ASSERT_EQ(v_0, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(false, it10->next(&t)) << "Iterator should be finished";

    // Request versions for ? ? <a> with offset 1
    TripleVersionsIterator* it11 = controller->get_version(Triple("", "", "<a>", dict), 1);

    ASSERT_EQ(false, it11->next(&t)) << "Iterator should be finished";

    // Request versions for ? ? <b>
    ASSERT_EQ(1, controller->get_version_count(Triple("", "", "<b>", dict)).first) << "Count is incorrect";
    TripleVersionsIterator* it12 = controller->get_version(Triple("", "", "<b>", dict), 0);

    ASSERT_EQ(true, it12->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <b>.", t.get_triple()->to_string(*dict)) << "Element is incorrect";
    ASSERT_EQ(v_0, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(false, it12->next(&t)) << "Iterator should be finished";

    // Request versions for ? ? <b> with offset 1
    TripleVersionsIterator* it13 = controller->get_version(Triple("", "", "<b>", dict), 1);

    ASSERT_EQ(false, it13->next(&t)) << "Iterator should be finished";

    // Request versions for ? ? <c>
    ASSERT_EQ(1, controller->get_version_count(Triple("", "", "<c>", dict)).first) << "Count is incorrect";
    TripleVersionsIterator* it14 = controller->get_version(Triple("", "", "<c>", dict), 0);

    ASSERT_EQ(true, it14->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <c>.", t.get_triple()->to_string(*dict)) << "Element is incorrect";
    ASSERT_EQ(v_0, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(false, it14->next(&t)) << "Iterator should be finished";

    // Request versions for ? ? <c> with offset 1
    TripleVersionsIterator* it15 = controller->get_version(Triple("", "", "<c>", dict), 1);

    ASSERT_EQ(false, it15->next(&t)) << "Iterator should be finished";

    // Request versions for ? ? <d>
    ASSERT_EQ(0, controller->get_version_count(Triple("", "", "<d>", dict)).first) << "Count is incorrect";
    TripleVersionsIterator* it16 = controller->get_version(Triple("", "", "<d>", dict), 0);

    ASSERT_EQ(false, it16->next(&t)) << "Iterator should be finished";
}

TEST_F(ControllerTest, GetVersion) {
    controller->new_patch_bulk()
            ->addition(hdt::TripleString("<a>", "<a>", "<a>"))
            ->addition(hdt::TripleString("<a>", "<a>", "<b>"))
            ->addition(hdt::TripleString("<a>", "<a>", "<c>"))
            ->commit();

    controller->new_patch_bulk()
            ->deletion(hdt::TripleString("<a>", "<a>", "<b>"))
            ->commit();

    controller->new_patch_bulk()
            ->addition(hdt::TripleString("<a>", "<a>", "<d>"))
            ->commit();

    std::shared_ptr<DictionaryManager> dict = controller->get_snapshot_manager()->get_dictionary_manager(0);

    // Expected version 0:
    // <a> <a> <a>
    // <a> <a> <b>
    // <a> <a> <c>

    // Expected version 1:
    // <a> <a> <a>
    // <a> <a> <c>

    // Expected version 2:
    // <a> <a> <a>
    // <a> <a> <c>
    // <a> <a> <d>

    TripleVersions t;

    std::vector<int> v_012(0);
    v_012.push_back(0);
    v_012.push_back(1);
    v_012.push_back(2);

    std::vector<int> v_0(0);
    v_0.push_back(0);

    std::vector<int> v_2(0);
    v_2.push_back(2);

    // Request versions for ? ? ?
    ASSERT_EQ(4, controller->get_version_count(Triple("", "", "", dict)).first) << "Count is incorrect";
    TripleVersionsIterator* it0 = controller->get_version(Triple("", "", "", dict), 0);

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <a>.", t.get_triple()->to_string(*dict)) << "Element is incorrect";
    ASSERT_EQ(v_012, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <b>.", t.get_triple()->to_string(*dict)) << "Element is incorrect";
    ASSERT_EQ(v_0, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <c>.", t.get_triple()->to_string(*dict)) << "Element is incorrect";
    ASSERT_EQ(v_012, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <d>.", t.get_triple()->to_string(*dict)) << "Element is incorrect";
    ASSERT_EQ(v_2, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(false, it0->next(&t)) << "Iterator should be finished";

    // Request versions for ? ? ? with offset 1
    TripleVersionsIterator* it1 = controller->get_version(Triple("", "", "", dict), 1);

    ASSERT_EQ(true, it1->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <b>.", t.get_triple()->to_string(*dict)) << "Element is incorrect";
    ASSERT_EQ(v_0, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(true, it1->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <c>.", t.get_triple()->to_string(*dict)) << "Element is incorrect";
    ASSERT_EQ(v_012, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(true, it1->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <d>.", t.get_triple()->to_string(*dict)) << "Element is incorrect";
    ASSERT_EQ(v_2, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(false, it1->next(&t)) << "Iterator should be finished";

    // Request versions for ? ? ? with offset 2
    TripleVersionsIterator* it2 = controller->get_version(Triple("", "", "", dict), 2);

    ASSERT_EQ(true, it2->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <c>.", t.get_triple()->to_string(*dict)) << "Element is incorrect";
    ASSERT_EQ(v_012, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(true, it2->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <d>.", t.get_triple()->to_string(*dict)) << "Element is incorrect";
    ASSERT_EQ(v_2, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(false, it2->next(&t)) << "Iterator should be finished";

    // Request versions for ? ? ? with offset 3
    TripleVersionsIterator* it3 = controller->get_version(Triple("", "", "", dict), 3);

    ASSERT_EQ(true, it3->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <d>.", t.get_triple()->to_string(*dict)) << "Element is incorrect";
    ASSERT_EQ(v_2, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(false, it3->next(&t)) << "Iterator should be finished";

    // Request versions for ? ? ? with offset 4
    TripleVersionsIterator* it4 = controller->get_version(Triple("", "", "", dict), 4);

    ASSERT_EQ(false, it4->next(&t)) << "Iterator should be finished";

    // Request versions for <a> ? ?
    ASSERT_EQ(4, controller->get_version_count(Triple("<a>", "", "", dict)).first) << "Count is incorrect";
    TripleVersionsIterator* it5 = controller->get_version(Triple("", "", "", dict), 0);

    ASSERT_EQ(true, it5->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <a>.", t.get_triple()->to_string(*dict)) << "Element is incorrect";
    ASSERT_EQ(v_012, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(true, it5->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <b>.", t.get_triple()->to_string(*dict)) << "Element is incorrect";
    ASSERT_EQ(v_0, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(true, it5->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <c>.", t.get_triple()->to_string(*dict)) << "Element is incorrect";
    ASSERT_EQ(v_012, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(true, it5->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <d>.", t.get_triple()->to_string(*dict)) << "Element is incorrect";
    ASSERT_EQ(v_2, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(false, it5->next(&t)) << "Iterator should be finished";

    // Request versions for <a> ? ? with offset 1
    TripleVersionsIterator* it6 = controller->get_version(Triple("<a>", "", "", dict), 1);

    ASSERT_EQ(true, it6->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <b>.", t.get_triple()->to_string(*dict)) << "Element is incorrect";
    ASSERT_EQ(v_0, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(true, it6->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <c>.", t.get_triple()->to_string(*dict)) << "Element is incorrect";
    ASSERT_EQ(v_012, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(true, it6->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <d>.", t.get_triple()->to_string(*dict)) << "Element is incorrect";
    ASSERT_EQ(v_2, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(false, it6->next(&t)) << "Iterator should be finished";

    // Request versions for <a> ? ? with offset 2
    TripleVersionsIterator* it7 = controller->get_version(Triple("<a>", "", "", dict), 2);

    ASSERT_EQ(true, it7->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <c>.", t.get_triple()->to_string(*dict)) << "Element is incorrect";
    ASSERT_EQ(v_012, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(true, it7->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <d>.", t.get_triple()->to_string(*dict)) << "Element is incorrect";
    ASSERT_EQ(v_2, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(false, it7->next(&t)) << "Iterator should be finished";

    // Request versions for <a> ? ? with offset 3
    TripleVersionsIterator* it8 = controller->get_version(Triple("<a>", "", "", dict), 3);

    ASSERT_EQ(true, it8->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <d>.", t.get_triple()->to_string(*dict)) << "Element is incorrect";
    ASSERT_EQ(v_2, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(false, it8->next(&t)) << "Iterator should be finished";

    // Request versions for <a> ? ? with offset 4
    TripleVersionsIterator* it9 = controller->get_version(Triple("<a>", "", "", dict), 4);

    ASSERT_EQ(false, it9->next(&t)) << "Iterator should be finished";

    // Request versions for ? ? <a>
    ASSERT_EQ(1, controller->get_version_count(Triple("", "", "<a>", dict)).first) << "Count is incorrect";
    TripleVersionsIterator* it10 = controller->get_version(Triple("", "", "<a>", dict), 0);

    ASSERT_EQ(true, it10->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <a>.", t.get_triple()->to_string(*dict)) << "Element is incorrect";
    ASSERT_EQ(v_012, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(false, it10->next(&t)) << "Iterator should be finished";

    // Request versions for ? ? <a> with offset 1
    TripleVersionsIterator* it11 = controller->get_version(Triple("", "", "<a>", dict), 1);

    ASSERT_EQ(false, it11->next(&t)) << "Iterator should be finished";

    // Request versions for ? ? <b>
    ASSERT_EQ(1, controller->get_version_count(Triple("", "", "<b>", dict)).first) << "Count is incorrect";
    TripleVersionsIterator* it12 = controller->get_version(Triple("", "", "<b>", dict), 0);

    ASSERT_EQ(true, it12->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <b>.", t.get_triple()->to_string(*dict)) << "Element is incorrect";
    ASSERT_EQ(v_0, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(false, it12->next(&t)) << "Iterator should be finished";

    // Request versions for ? ? <b> with offset 1
    TripleVersionsIterator* it13 = controller->get_version(Triple("", "", "<b>", dict), 1);

    ASSERT_EQ(false, it13->next(&t)) << "Iterator should be finished";

    // Request versions for ? ? <c>
    ASSERT_EQ(1, controller->get_version_count(Triple("", "", "<c>", dict)).first) << "Count is incorrect";
    TripleVersionsIterator* it14 = controller->get_version(Triple("", "", "<c>", dict), 0);

    ASSERT_EQ(true, it14->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <c>.", t.get_triple()->to_string(*dict)) << "Element is incorrect";
    ASSERT_EQ(v_012, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(false, it14->next(&t)) << "Iterator should be finished";

    // Request versions for ? ? <c> with offset 1
    TripleVersionsIterator* it15 = controller->get_version(Triple("", "", "<c>", dict), 1);

    ASSERT_EQ(false, it15->next(&t)) << "Iterator should be finished";

    // Request versions for ? ? <d>
    ASSERT_EQ(1, controller->get_version_count(Triple("", "", "<d>", dict)).first) << "Count is incorrect";
    TripleVersionsIterator* it16 = controller->get_version(Triple("", "", "<d>", dict), 0);

    ASSERT_EQ(true, it16->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <d>.", t.get_triple()->to_string(*dict)) << "Element is incorrect";
    ASSERT_EQ(v_2, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(false, it16->next(&t)) << "Iterator should be finished";

    // Request versions for ? ? <d> with offset 1
    TripleVersionsIterator* it17 = controller->get_version(Triple("", "", "<d>", dict), 1);

    ASSERT_EQ(false, it17->next(&t)) << "Iterator should be finished";
}


TEST_F(ControllerMSTest, GetVersionMS) {
    controller->new_patch_bulk()
    ->addition(hdt::TripleString("<a>", "<a>", "<a>"))
    ->addition(hdt::TripleString("<a>", "<a>", "<b>"))
    ->addition(hdt::TripleString("<a>", "<a>", "<c>"))
    ->commit();

    controller->new_patch_bulk()
    ->deletion(hdt::TripleString("<a>", "<a>", "<b>"))
    ->commit();

    controller->new_patch_bulk()
    ->addition(hdt::TripleString("<a>", "<a>", "<d>"))
    ->commit();

    controller->new_patch_bulk()
    ->addition(hdt::TripleString("<a>", "<b>", "<c>"))
    ->addition(hdt::TripleString("<a>", "<b>", "<d>"))
    ->deletion(hdt::TripleString("<a>", "<a>", "<a>"))
    ->commit();

    // Expected version 0:
    // <a> <a> <a>
    // <a> <a> <b>
    // <a> <a> <c>

    // Expected version 1:
    // <a> <a> <a>
    // <a> <a> <c>

    // Expected version 2:
    // <a> <a> <a>
    // <a> <a> <c>
    // <a> <a> <d>

    // Expected version 3
    // <a> <a> <c>
    // <a> <a> <d>
    // <a> <b> <c>
    // <a> <b> <d>

    TripleVersions t;

    std::vector<int> v_012 = {0,1,2};
    std::vector<int> v_0123 = {0,1,2,3};
    std::vector<int> v_0 = {0};
    std::vector<int> v_23 = {2,3};
    std::vector<int> v_3 = {3};

    // Request versions for ? ? ?
    ASSERT_EQ(6, controller->get_version_count(StringTriple("", "", "")).first) << "Count is incorrect";
    TripleVersionsIterator* it0 = controller->get_version(StringTriple("", "", ""), 0);

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <a>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(v_012, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <b>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(v_0, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <c>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(v_0123, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <d>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(v_23, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <b> <c>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(v_3, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(true, it0->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <b> <d>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(v_3, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(false, it0->next(&t)) << "Iterator should be finished";

    // Request versions for ? ? ? with offset 1
    TripleVersionsIterator* it1 = controller->get_version(StringTriple("", "", ""), 1);

    ASSERT_EQ(true, it1->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <b>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(v_0, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(true, it1->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <c>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(v_0123, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(true, it1->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <d>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(v_23, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(true, it1->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <b> <c>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(v_3, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(true, it1->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <b> <d>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(v_3, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(false, it1->next(&t)) << "Iterator should be finished";

    // Request versions for ? ? ? with offset 2
    TripleVersionsIterator* it2 = controller->get_version(StringTriple("", "", ""), 2);

    ASSERT_EQ(true, it2->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <c>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(v_0123, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(true, it2->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <d>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(v_23, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(true, it2->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <b> <c>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(v_3, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(true, it2->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <b> <d>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(v_3, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(false, it2->next(&t)) << "Iterator should be finished";

    // Request versions for ? ? ? with offset 3
    TripleVersionsIterator* it3 = controller->get_version(StringTriple("", "", ""), 3);

    ASSERT_EQ(true, it3->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <d>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(v_23, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(true, it3->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <b> <c>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(v_3, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(true, it3->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <b> <d>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(v_3, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(false, it3->next(&t)) << "Iterator should be finished";

    // Request versions for ? ? ? with offset 6
    TripleVersionsIterator* it4 = controller->get_version(StringTriple("", "", ""), 6);

    ASSERT_EQ(false, it4->next(&t)) << "Iterator should be finished";

    // Request versions for <a> ? ?
    ASSERT_EQ(6, controller->get_version_count(StringTriple("<a>", "", "")).first) << "Count is incorrect";
    TripleVersionsIterator* it5 = controller->get_version(StringTriple("", "", ""), 0);

    ASSERT_EQ(true, it5->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <a>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(v_012, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(true, it5->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <b>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(v_0, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(true, it5->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <c>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(v_0123, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(true, it5->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <d>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(v_23, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(true, it5->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <b> <c>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(v_3, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(true, it5->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <b> <d>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(v_3, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(false, it5->next(&t)) << "Iterator should be finished";

    // Request versions for <a> ? ? with offset 1
    TripleVersionsIterator* it6 = controller->get_version(StringTriple("<a>", "", ""), 1);

    ASSERT_EQ(true, it6->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <b>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(v_0, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(true, it6->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <c>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(v_0123, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(true, it6->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <d>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(v_23, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(true, it6->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <b> <c>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(v_3, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(true, it6->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <b> <d>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(v_3, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(false, it6->next(&t)) << "Iterator should be finished";

    // Request versions for <a> ? ? with offset 2
    TripleVersionsIterator* it7 = controller->get_version(StringTriple("<a>", "", ""), 2);

    ASSERT_EQ(true, it7->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <c>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(v_0123, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(true, it7->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <d>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(v_23, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(true, it7->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <b> <c>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(v_3, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(true, it7->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <b> <d>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(v_3, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(false, it7->next(&t)) << "Iterator should be finished";

    // Request versions for <a> ? ? with offset 3
    TripleVersionsIterator* it8 = controller->get_version(StringTriple("<a>", "", ""), 3);

    ASSERT_EQ(true, it8->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <d>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(v_23, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(true, it8->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <b> <c>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(v_3, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(true, it8->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <b> <d>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(v_3, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(false, it8->next(&t)) << "Iterator should be finished";

    // Request versions for <a> ? ? with offset 6
    TripleVersionsIterator* it9 = controller->get_version(StringTriple("<a>", "", ""), 6);

    ASSERT_EQ(false, it9->next(&t)) << "Iterator should be finished";

    // Request versions for ? ? <a>
    ASSERT_EQ(1, controller->get_version_count(StringTriple("", "", "<a>")).first) << "Count is incorrect";
    TripleVersionsIterator* it10 = controller->get_version(StringTriple("", "", "<a>"), 0);

    ASSERT_EQ(true, it10->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <a>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(v_012, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(false, it10->next(&t)) << "Iterator should be finished";

    // Request versions for ? ? <a> with offset 1
    TripleVersionsIterator* it11 = controller->get_version(StringTriple("", "", "<a>"), 1);

    ASSERT_EQ(false, it11->next(&t)) << "Iterator should be finished";

    // Request versions for ? ? <b>
    ASSERT_EQ(1, controller->get_version_count(StringTriple("", "", "<b>")).first) << "Count is incorrect";
    TripleVersionsIterator* it12 = controller->get_version(StringTriple("", "", "<b>"), 0);

    ASSERT_EQ(true, it12->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <b>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(v_0, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(false, it12->next(&t)) << "Iterator should be finished";

    // Request versions for ? ? <b> with offset 1
    TripleVersionsIterator* it13 = controller->get_version(StringTriple("", "", "<b>"), 1);

    ASSERT_EQ(false, it13->next(&t)) << "Iterator should be finished";

    // Request versions for ? ? <c>
    ASSERT_EQ(2, controller->get_version_count(StringTriple("", "", "<c>")).first) << "Count is incorrect";
    TripleVersionsIterator* it14 = controller->get_version(StringTriple("", "", "<c>"), 0);

    ASSERT_EQ(true, it14->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <c>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(v_0123, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(true, it14->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <b> <c>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(v_3, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(false, it14->next(&t)) << "Iterator should be finished";

    // Request versions for ? ? <c> with offset 1
    TripleVersionsIterator* it15 = controller->get_version(StringTriple("", "", "<c>"), 1);

    ASSERT_EQ(true, it15->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <b> <c>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(v_3, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(false, it15->next(&t)) << "Iterator should be finished";

    // Request versions for ? ? <c> with offset 2

    TripleVersionsIterator* it16 = controller->get_version(StringTriple("", "", "<c>"), 2);

    ASSERT_EQ(false, it16->next(&t)) << "Iterator should be finished";


    // Request versions for ? ? <d>
    ASSERT_EQ(2, controller->get_version_count(StringTriple("", "", "<d>")).first) << "Count is incorrect";
    TripleVersionsIterator* it17 = controller->get_version(StringTriple("", "", "<d>"), 0);

    ASSERT_EQ(true, it17->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <a> <d>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(v_23, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(true, it17->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <b> <d>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(v_3, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(false, it17->next(&t)) << "Iterator should be finished";

    // Request versions for ? ? <d> with offset 1
    TripleVersionsIterator* it18 = controller->get_version(StringTriple("", "", "<d>"), 1);

    ASSERT_EQ(true, it18->next(&t)) << "Iterator has a no next value";
    ASSERT_EQ("<a> <b> <d>.", t.get_triple()->to_string(*(t.get_dictionary()))) << "Element is incorrect";
    ASSERT_EQ(v_3, *(t.get_versions())) << "Element is incorrect";

    ASSERT_EQ(false, it18->next(&t)) << "Iterator should be finished";

    // Request versions for ? ? <d> with offset 2
    TripleVersionsIterator* it19 = controller->get_version(StringTriple("", "", "<d>"), 2);

    ASSERT_EQ(false, it19->next(&t)) << "Iterator should be finished";

}
