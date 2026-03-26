#include <gtest/gtest.h>

#include <sstream>

#include "io/csv_reader.h"

TEST(CSVReaderTest, ParsesRow) {
  std::istringstream in("a,b,c\n1,2,3\n");
  CSVIterator it(in);
  EXPECT_EQ(std::string((*it)[0]), "a");
  ++it;
  EXPECT_EQ(std::string((*it)[2]), "3");
}

TEST(CSVReaderTest, RowOutOfRangeThrows) {
  std::istringstream in("a,b\n");
  CSVIterator it(in);
  EXPECT_THROW((*it)[3], std::out_of_range);
}

TEST(CSVReaderTest, HandlesCRLFLineEndings) {
  std::istringstream in("a,b\r\n1,2\r\n");
  CSVIterator it(in);
  EXPECT_EQ(std::string((*it)[0]), "a");
  ++it;
  EXPECT_EQ(std::string((*it)[0]), "1");
  EXPECT_EQ(std::string((*it)[1]), "2");
}

TEST(CSVReaderTest, EmptyStreamHasNoRows) {
  std::istringstream in("");
  CSVRange range(in);
  auto begin = range.begin();
  auto end = range.end();
  EXPECT_TRUE(begin == end);
}

TEST(CSVReaderTest, ShortMalformedRowStillParseableBySize) {
  std::istringstream in("a,b,c,d,e\n1,2\n");
  CSVIterator it(in);
  ++it;
  EXPECT_EQ((*it).size(), 2u);
  EXPECT_EQ(std::string((*it)[0]), "1");
  EXPECT_EQ(std::string((*it)[1]), "2");
}

TEST(CSVReaderTest, MixedLineEndingsRemainParseable) {
  std::istringstream in("a,b\n1,2\r\n3,4\n");
  CSVIterator it(in);
  EXPECT_EQ(std::string((*it)[0]), "a");
  ++it;
  EXPECT_EQ(std::string((*it)[0]), "1");
  EXPECT_EQ(std::string((*it)[1]), "2");
  ++it;
  EXPECT_EQ(std::string((*it)[0]), "3");
  EXPECT_EQ(std::string((*it)[1]), "4");
}

TEST(CSVReaderTest, EmptyMiddleFieldIsPreserved) {
  std::istringstream in("a,b,c\n1,,3\n");
  CSVIterator it(in);
  ++it;
  EXPECT_EQ((*it).size(), 3u);
  EXPECT_EQ(std::string((*it)[0]), "1");
  EXPECT_EQ(std::string((*it)[1]), "");
  EXPECT_EQ(std::string((*it)[2]), "3");
}

TEST(CSVReaderTest, TrailingBlankLineProducesEmptySingleFieldRow) {
  std::istringstream in("a,b\n1,2\n\n");
  CSVIterator it(in);
  ++it;
  EXPECT_EQ((*it).size(), 2u);
  ++it;
  EXPECT_EQ((*it).size(), 1u);
  EXPECT_EQ(std::string((*it)[0]), "");
}
