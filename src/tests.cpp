#include <gtest/gtest.h>

#include "FXML.h"

std::string_view constexpr SIMPLE_DATA_FILEPATH = "test_data/simple_data.xml";

using namespace fxml;

struct FXMLTests : ::testing::Test
{
	void SetUp() override
	{

	}
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
}