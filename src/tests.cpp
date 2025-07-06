#include <gtest/gtest.h>

#include <array>

#include "FXML.h"

std::string_view constexpr SIMPLE_DATA_FILEPATH = "test_data/simple_data.xml";
std::array<std::string_view, 6> constexpr SIMPLE_DATA_NODE_NAMES = {"root", "node_one", "node_two", "name", "country", "node_three"};

using namespace fxml;

struct FXMLTests : ::testing::Test
{
  void SetUp() override {}
};

TEST_F(FXMLTests, testWrongFilepath)
{
  EXPECT_EQ(XMLParser{}.Parse("BlablaBla").error().reason(), ErrorReason::CANNOT_FIND_FILE);
  EXPECT_FALSE(XMLParser{}.Parse("BlaBlaBla").has_value());
}

TEST_F(FXMLTests, testParseSimpleFile)
{
  XMLParser parser{};
  auto ret = parser.Parse(SIMPLE_DATA_FILEPATH);
  EXPECT_TRUE(ret.has_value());

  if (!ret.has_value())
  {
    std::cout << "Error: " << ret.error().what() << "\n";
  }

  XMLDocument doc{ret.value()};

  // Check the Tags
  for (size_t i{}; i < doc.GetNrOfNodes(); ++i)
  {
    EXPECT_EQ(doc.GetNodeByIndex(i).value().get().tag.name, SIMPLE_DATA_NODE_NAMES[i]);
  }

  // Check content
  EXPECT_EQ(doc.GetNodeByName("node_one").value().get().content, "This is an XML Parser");
  EXPECT_EQ(doc.GetNodeByName("name").value().get().content, "Rhidian");
  EXPECT_EQ(doc.GetNodeByName("country").value().get().content, "Belgium");
}