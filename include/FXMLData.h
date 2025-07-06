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
    std::vector<XMLTag> children;
    std::string_view content;
  };

  class XMLDocument
  {
   private:
    std::vector<XMLElement> m_elements;

   public:
    void AddXMLElement(XMLElement const&& element);

    std::optional<std::reference_wrapper<XMLTag const>> FindTag(std::string_view tag);
  };
}  // namespace fxml