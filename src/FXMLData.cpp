#include "FXMLData.h"

namespace fxml
{
	void XMLDocument::AddXMLElement(XMLElement const&& element)
	{
		m_elements.push_back(element);
	}

	std::optional<std::reference_wrapper<const XMLTag>> XMLDocument::FindTag(std::string_view tag)
	{
		for (const XMLElement& element : m_elements)
		{
			if (element.tag.name == tag)
			{
				return std::cref(element.tag);
			}
		}

		return std::nullopt;
	}
} // namespace fxml