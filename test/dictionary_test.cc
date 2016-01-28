//#include "cpp_sorter.h"
#include "../src/main/cpp/dictionary/dictionary.cc"
#include <HDTEnums.hpp>
#include <gtest/gtest.h>

TEST(DictionaryManagerTest, Insert) {
  DictionaryManager *dict = new DictionaryManager(NULL, NULL);
  unsigned int id =
      *dict->insert("http://example.org", TripleComponentRole.SUBJECT);

  ASSERT_EQ(1, id);
}
