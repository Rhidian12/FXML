#pragma once

#include <string_view>
#include <vector>

namespace fxml
{
	struct XMLTag
	{
		std::string_view name;
		std::vector<std::string_view> attributes;
	};

	class XMLDocument
	{
	private:
		std::vector<XMLTag> m_tags;

	public:
		void AddTag(XMLTag const&& tag);
	};
} // namespace fxml