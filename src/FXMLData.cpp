#include "FXMLData.h"

namespace fxml
{
  XMLElement::XMLElement(XMLTag const&& tag)
    : m_tag(std::move(tag))
    , m_children()
    , m_rawContent()
  {
  }

  void XMLElement::SetRawContent(std::string_view content)
  {
    m_rawContent = content;
  }

  XMLTag const& XMLElement::GetTag() const
  {
    return m_tag;
  }

  std::string_view XMLElement::GetRawContent() const
  {
    return m_rawContent;
  }

  void XMLDocument::AddXMLElement(XMLElement const&& element, bool front)
  {
    if (front)
    {
      m_elements.insert(m_elements.begin(), std::move(element));
    }
    else
    {
      m_elements.push_back(std::move(element));
    }
  }

  std::optional<std::reference_wrapper<XMLElement const>> XMLDocument::GetNodeByName(std::string_view tagName)
  {
    for (XMLElement const& element : m_elements)
    {
      if (element.GetTag().name == tagName)
      {
        return element;
      }
    }

    return std::nullopt;
  }

  std::optional<std::reference_wrapper<XMLElement const>> XMLDocument::GetNodeByIndex(uint32_t index) const
  {
    if (index >= m_elements.size())
    {
      return std::nullopt;
    }

    return m_elements[index];
  }

  size_t XMLDocument::GetNrOfNodes() const
  {
    return m_elements.size();
  }
}  // namespace fxml