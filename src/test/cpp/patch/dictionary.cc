//#include "cpp_sorter.h"
#include "../../../main/cpp/dictionary/dictionary.cc"
#include <HDTEnums.hpp>
#include <HDTSpecification.hpp>
#include <gtest/gtest.h>

// using namespace hdt;

DictionaryManager *dict = new DictionaryManager();

TEST(InsertTest, Insert) {

  TripleComponentRole role = SUBJECT;
  std::string val = "http://example.org";
  unsigned int id = dict->insert(val, role);

  ASSERT_EQ(1, id);
}

TEST(PatchDictionaryTest, Insert) {

  TripleComponentRole role = SUBJECT;
  std::string val = "http://example.org";

  unsigned int id = dict->stringToId(val, role);

  ASSERT_EQ(1, id);

  std::string str = dict->idToString(id, role);

  ASSERT_EQ(val, str);
}
