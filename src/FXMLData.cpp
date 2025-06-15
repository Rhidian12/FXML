#include "FXMLData.h"

namespace fxml
{
	void XMLDocument::AddTag(XMLTag const&& tag)
	{
		m_tags.push_back(std::move(tag));
	}
} // namespace fxml