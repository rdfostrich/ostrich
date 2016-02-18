//#include "cpp_sorter.h"
#include "../../../main/cpp/dictionary/dictionary.cc"
#include <HDTEnums.hpp>
#include <HDTSpecification.hpp>
#include <gtest/gtest.h>

// using namespace hdt;

DictionaryManager *dict = new DictionaryManager();

std::string a = "http://example.org/a";
std::string b = "http://example.org/b";
std::string c = "http://example.org/c";

unsigned int patch_id_a = 2147483649;
unsigned int patch_id_b = 2147483650;
unsigned int patch_id_c = 2147483651;

TEST(PathDictionaryInsertTest, Insert) {

  TripleComponentRole role = SUBJECT;

  unsigned int idA = dict->insert(a, role);
  unsigned int idB = dict->insert(b, role);
  unsigned int idC = dict->insert(c, role);

  EXPECT_EQ(patch_id_a, idA);
  EXPECT_EQ(patch_id_b, idB);
  EXPECT_EQ(patch_id_c, idC);
}

TEST(PatchDictionaryLookupTest, Insert) {

  TripleComponentRole role = SUBJECT;

  unsigned int idA = dict->stringToId(a, role);
  unsigned int idB = dict->stringToId(b, role);
  unsigned int idC = dict->stringToId(c, role);

  EXPECT_EQ(patch_id_a, idA);
  EXPECT_EQ(patch_id_b, idB);
  EXPECT_EQ(patch_id_c, idC);

  std::string strA = dict->idToString(patch_id_a, role);
  std::string strB = dict->idToString(patch_id_b, role);
  std::string strC = dict->idToString(patch_id_c, role);

  EXPECT_EQ(a, strA);
  EXPECT_EQ(b, strB);
  EXPECT_EQ(c, strC);
}
