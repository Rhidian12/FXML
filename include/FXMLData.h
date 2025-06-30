#pragma once

#include <map>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>

namespace fxml
{
	struct XMLElement;

	struct XMLTag
	{
		std::string_view name;
		std::map<std::string_view, std::string_view> attributes;
		XMLElement* content = nullptr;
	};

	struct XMLElement
	{
		XMLTag tag;
		std::vector<XMLTag> children;
	};

	class XMLDocument
	{
	private:
		std::vector<XMLElement> m_elements;

	public:
		void AddXMLElement(XMLElement const && element);

		std::optional<std::reference_wrapper<const XMLTag>> FindTag(std::string_view tag);
	};
} // namespace fxml