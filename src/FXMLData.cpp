#include "FXMLData.h"

namespace fxml
{
  void XMLDocument::AddXMLElement(XMLElement const&& element) { m_elements.push_back(element); }

  std::optional<std::reference_wrapper<XMLTag const>> XMLDocument::FindTag(std::string_view tag)
  {
    for (XMLElement const& element : m_elements)
    {
      if (element.tag.name == tag)
      {
        return std::cref(element.tag);
      }
    }

    return std::nullopt;
  }
}  // namespace fxml