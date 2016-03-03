//#include "cpp_sorter.h"
#include "../../../main/cpp/dictionary/dictionary_manager.h"
#include <HDTEnums.hpp>
#include <HDTSpecification.hpp>
#include <dictionary/PlainDictionary.hpp>
#include <gtest/gtest.h>

// using namespace hdt;

DictionaryManager *dict = new DictionaryManager();

std::string a = "http://example.org/a";
std::string b = "http://example.org/b";
std::string c = "http://example.org/c";
std::string d = "http://example.org/d";

unsigned int patch_id_a = 2147483649;
unsigned int patch_id_b = 2147483650;
unsigned int patch_id_c = 2147483651;

unsigned int hdt_id_a = 1;
unsigned int hdt_id_b = 2;
unsigned int hdt_id_c = 3;

TripleComponentRole role = SUBJECT;

TEST(PathDictionaryInsertTest, Insert) {
  unsigned int idA = dict->insert(a, role);
  unsigned int idB = dict->insert(b, role);
  unsigned int idC = dict->insert(c, role);

  EXPECT_EQ(patch_id_a, idA);
  EXPECT_EQ(patch_id_b, idB);
  EXPECT_EQ(patch_id_c, idC);
}

TEST(PatchDictionaryLookupTest, Insert) {
  // Should return the correct ids
  unsigned int idA = dict->stringToId(a, role);
  unsigned int idB = dict->stringToId(b, role);
  unsigned int idC = dict->stringToId(c, role);

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
}

TEST(SnapshotDictionaryTest, Insert) {
  ModifiableDictionary *hdtDict = new PlainDictionary();
  hdtDict->insert(a, role);
  hdtDict->insert(b, role);
  hdtDict->insert(c, role);

  delete dict;
  dict = new DictionaryManager(hdtDict);

  unsigned int idA = dict->stringToId(a, role);
  unsigned int idB = dict->stringToId(b, role);
  unsigned int idC = dict->stringToId(c, role);

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
  unsigned int idA_copy = dict->insert(a, role);
  EXPECT_EQ(hdt_id_a, idA_copy);

  // Should return first patch id
  unsigned int idD = dict->insert(d, role);
  EXPECT_EQ(patch_id_a, idD);
}
