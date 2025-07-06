#pragma once

#include <map>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>

namespace fxml
{
  /*
  <node name="FXML">
          <child>This is content</child>
          <foo><!--Comment!-->AndContent!</foo>
  </node>
  */
  struct XMLTag
  {
    std::string_view name;
    std::map<std::string_view, std::string_view> attributes;
  };

  struct XMLElement
  {
    XMLTag tag;
    std::vector<XMLElement> children;
    std::string_view content;
  };

  class XMLDocument
  {
   private:
    friend class XMLParser;

   private:
    std::vector<XMLElement> m_elements;

   public:
    std::optional<std::reference_wrapper<XMLElement const>> GetNodeByName(std::string_view tagName);
    std::optional<std::reference_wrapper<XMLElement const>> GetNodeByIndex(uint32_t index) const;

    size_t GetNrOfNodes() const;

   private:
    void AddXMLElement(XMLElement const&& element, bool front);
  };
}  // namespace fxml