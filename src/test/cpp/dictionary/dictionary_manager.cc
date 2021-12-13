//#include "cpp_sorter.h"
#include "../../../main/cpp/dictionary/dictionary_manager.h"
#include "../../../main/cpp/snapshot/vector_triple_iterator.h"
#include <HDTEnums.hpp>
#include <HDTSpecification.hpp>
#include <HDTManager.hpp>
#include <hdt/BasicHDT.hpp>
#include <dictionary/PlainDictionary.hpp>
#include <gtest/gtest.h>

#define TESTPATH "./"

using namespace hdt;

class DictionaryManagerTest : public ::testing::Test {
protected:
    DictionaryManager *dict;
    TripleComponentRole role;

    std::string a, b, c, d, e, f, g, h, i, literal, literal2, literal3, empty;

    size_t patch_id_a;
    size_t patch_id_b;
    size_t patch_id_c;

    size_t hdt_id_a;
    size_t hdt_id_b;
    size_t hdt_id_c;

    DictionaryManagerTest() {}

    virtual void SetUp() {
        std::remove((TESTPATH + PATCHDICT_FILENAME_BASE(0)).c_str());
        dict = new DictionaryManager(TESTPATH, 0);

        a = "http://example.org/a";
        b = "http://example.org/b";
        c = "http://example.org/c";
        d = "http://example.org/d";
        e = "http://example.org/e";
        f = "http://example.org/f";
        g = "http://example.org/g";
        h = "http://example.org/h";
        i = "http://example.org/i";
        empty = "",
        literal = "\"aaaaaa\"^^xsd:string",
        literal2 = "\"This is a second literal.\"^^xsd:string",
        literal3 = "\"zzzzzz\"^^xsd:string";

        size_t bitmask = 1ULL << (8 * sizeof(size_t) - 1);
        patch_id_a = bitmask +1;
        patch_id_b = bitmask +2;
        patch_id_c = bitmask +3;

        hdt_id_a = 1;
        hdt_id_b = 2;
        hdt_id_c = 3;

        role = SUBJECT;
    }

    virtual void TearDown() {
        std::remove((TESTPATH + PATCHDICT_FILENAME_BASE(0)).c_str());
    }
};

TEST_F(DictionaryManagerTest, Insert) {
    size_t idA = dict->insert(a, role);
    size_t idB = dict->insert(b, role);
    size_t idC = dict->insert(c, role);

    EXPECT_EQ(patch_id_a, idA);
    EXPECT_EQ(patch_id_b, idB);
    EXPECT_EQ(patch_id_c, idC);

    // Check if variable are handled properly
    EXPECT_EQ(0, dict->insert(empty, role));
}

TEST_F(DictionaryManagerTest, Lookup) {
    dict->insert(a, role);
    dict->insert(b, role);
    dict->insert(c, role);

    // Should return the correct ids
    size_t idA = dict->stringToId(a, role);
    size_t idB = dict->stringToId(b, role);
    size_t idC = dict->stringToId(c, role);

    EXPECT_EQ(patch_id_a, idA);
    EXPECT_EQ(patch_id_b, idB);
    EXPECT_EQ(patch_id_c, idC);

    // Should return the correct strings
    std::string strA = dict->idToString(patch_id_a, role);
    std::string strB = dict->idToString(patch_id_b, role);
    std::string strC = dict->idToString(patch_id_c, role);

    EXPECT_EQ(a, strA);
    EXPECT_EQ(b, strB);
    EXPECT_EQ(c, strC);

    // Check if variable are handled properly
    EXPECT_EQ(0, dict->stringToId(empty, role));
    EXPECT_EQ(empty, dict->idToString(0, role));
}

TEST_F(DictionaryManagerTest, SnapshotDictionary) {
    ModifiableDictionary *hdtDict = new PlainDictionary();
    hdtDict->insert(a, role);
    hdtDict->insert(b, role);
    hdtDict->insert(c, role);

    delete dict;
    dict = new DictionaryManager(TESTPATH, 0, hdtDict);

    size_t idA = dict->stringToId(a, role);
    size_t idB = dict->stringToId(b, role);
    size_t idC = dict->stringToId(c, role);

    EXPECT_EQ(hdt_id_a, idA);
    EXPECT_EQ(hdt_id_b, idB);
    EXPECT_EQ(hdt_id_c, idC);

    std::string strA = dict->idToString(hdt_id_a, role);
    std::string strB = dict->idToString(hdt_id_b, role);
    std::string strC = dict->idToString(hdt_id_c, role);

    EXPECT_EQ(a, strA);
    EXPECT_EQ(b, strB);
    EXPECT_EQ(c, strC);

    // Should not insert duplicate
    size_t idA_copy = dict->insert(a, role);
    EXPECT_EQ(hdt_id_a, idA_copy);

    // Should return first patch id
    size_t idD = dict->insert(d, role);
    EXPECT_EQ(patch_id_a, idD);
}

TEST_F(DictionaryManagerTest, HdtDictionary) {
    // Build a snapshot
    string fileName = "temp.hdt";

    std::vector<TripleString> triples;
    triples.push_back(TripleString(a, h, literal));
    triples.push_back(TripleString(a, a, a));
    triples.push_back(TripleString(a, a, b));
    triples.push_back(TripleString(a, a, c));
    triples.push_back(TripleString(a, b, c));
    triples.push_back(TripleString(a, b, literal2));
    triples.push_back(TripleString(e, f, g));
    triples.push_back(TripleString(a, h, i));
    triples.push_back(TripleString(c, f, h));
    triples.push_back(TripleString(a, f, i));
    triples.push_back(TripleString(a, h, literal3));
    VectorTripleIterator *it = new VectorTripleIterator(triples);

    BasicHDT *basicHdt = new BasicHDT();
    basicHdt->loadFromTriples(it, "<http://example.org>");
    basicHdt->saveToHDT((TESTPATH + fileName).c_str());
    HDT *snapshot = hdt::HDTManager::loadHDT((TESTPATH + fileName).c_str());

    delete dict;
    dict = new DictionaryManager(TESTPATH, 0, snapshot->getDictionary());

    // Order of insertion: a e f g h i literal i b c literal2
    // Presumed order: s (a c e) o(literal2 literal literal3 ; b g h i)
    // s+o: a b c e g h i literal literal2
    size_t idA_S = dict->stringToId(a, SUBJECT);
    size_t idE = dict->stringToId(e, SUBJECT);
    size_t idC_S = dict->stringToId(c, SUBJECT);

    size_t idLiteral = dict->stringToId(literal, OBJECT);
    size_t idLiteral2 = dict->stringToId(literal2, OBJECT);
    size_t idLiteral3 = dict->stringToId(literal3, OBJECT);

    size_t idA_O = dict->stringToId(a, OBJECT);
    size_t idG = dict->stringToId(g, OBJECT);
    size_t idH = dict->stringToId(h, OBJECT);
    size_t idI = dict->stringToId(i, OBJECT);
    size_t idB = dict->stringToId(b, OBJECT);
    size_t idC_O = dict->stringToId(c, OBJECT);


    EXPECT_EQ(1, idA_S);
    EXPECT_EQ(2, idC_S);
    EXPECT_EQ(3, idE);

    // literals are stored seperately
    EXPECT_EQ(3, idLiteral2);
    EXPECT_EQ(4, idLiteral);
    EXPECT_EQ(5, idLiteral3);

    EXPECT_EQ(6, idB);
    EXPECT_EQ(7, idG);
    EXPECT_EQ(8, idH);
    EXPECT_EQ(9, idI);

// was added as subject before
    EXPECT_EQ(1, idA_O);
    EXPECT_EQ(2, idC_O);


    // p: a f h
    // predicates are stored seperately and sorted
    size_t idA2 = dict->stringToId(a, PREDICATE);
    size_t idB2 = dict->stringToId(b, PREDICATE);
    size_t idF = dict->stringToId(f, PREDICATE);
    size_t idH2 = dict->stringToId(h, PREDICATE);

    EXPECT_EQ(1, idA2);
    EXPECT_EQ(2, idB2);
    EXPECT_EQ(3, idF);
    EXPECT_EQ(4, idH2);

    std::string strA = dict->idToString(1, OBJECT);
    std::string strB = dict->idToString(6, OBJECT);
    std::string strC = dict->idToString(2, OBJECT);

    EXPECT_EQ(a, strA);
    EXPECT_EQ(b, strB);
    EXPECT_EQ(c, strC);

    // Should not insert duplicate
    size_t idA1_copy = dict->insert(a, SUBJECT);
    size_t idA2_copy = dict->insert(a, PREDICATE);
    size_t idA3_copy = dict->insert(a, OBJECT);
    EXPECT_EQ(hdt_id_a, idA1_copy);
    EXPECT_EQ(hdt_id_a, idA2_copy);
    EXPECT_EQ(hdt_id_a, idA3_copy);

    // Should return first patch id
    size_t idD = dict->insert(d, SUBJECT);
    size_t idD2 = dict->insert(d, OBJECT);
    EXPECT_EQ(patch_id_a, idD);
    EXPECT_EQ(patch_id_a, idD2);

    // Check if variable are handled properly
    EXPECT_EQ(0, dict->insert(empty, role));
    EXPECT_EQ(0, dict->stringToId(empty, role));
    EXPECT_EQ(empty, dict->idToString(0, role));

    remove(fileName.c_str());
    remove((fileName + ".index").c_str());
}

TEST_F(DictionaryManagerTest, SaveAndLoad) {
    delete dict;
    dict = new DictionaryManager(TESTPATH, 0);
    dict->insert(a, SUBJECT);
    dict->insert(b, PREDICATE);
    dict->insert(c, OBJECT);
    dict->insert(d, SUBJECT);
    dict->insert(e, PREDICATE);
    dict->insert(f, OBJECT);
    dict->insert(g, SUBJECT);
    dict->insert(h, PREDICATE);
    dict->insert(i, OBJECT);

    delete dict;
    dict = new DictionaryManager(TESTPATH, 0);

    size_t base = 1ULL << 63;

    EXPECT_EQ(base+1, dict->stringToId(a, SUBJECT));
    EXPECT_EQ(base+1, dict->stringToId(b, PREDICATE));
    EXPECT_EQ(base+1, dict->stringToId(c, OBJECT));
    EXPECT_EQ(base+2, dict->stringToId(d, SUBJECT));
    EXPECT_EQ(base+2, dict->stringToId(e, PREDICATE));
    EXPECT_EQ(base+2, dict->stringToId(f, OBJECT));
    EXPECT_EQ(base+3, dict->stringToId(g, SUBJECT));
    EXPECT_EQ(base+3, dict->stringToId(h, PREDICATE));
    EXPECT_EQ(base+3, dict->stringToId(i, OBJECT));
}
