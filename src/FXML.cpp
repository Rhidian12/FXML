#include "FXML.h"

#include <array>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <limits>
#include <ranges>

#include "FXMLData.h"

#if defined(__GNUC__) || defined(__GNUG__)
#define FORCE_INLINE __attribute__((always_inline)) inline
#elif defined(_MSC_VER)
#define FORCE_INLINE __forceinline
#endif

namespace fxml
{
  namespace
  {
#define CHECK_EXPECTED_VOID(expected)    \
  if (auto const res = (expected); !res) \
  {                                      \
    return std::unexpected{res.error()}; \
  }
#define CHECK_EXPECTED(type, name, expected, errorMsg)       \
  type name;                                                 \
  if (auto const res = (expected); !res)                     \
  {                                                          \
    return std::unexpected{XMLError{res.error(), errorMsg}}; \
  }                                                          \
  else                                                       \
  {                                                          \
    name = res.value();                                      \
  }
#define CHECK_EXPECTED_NO_TRANSFORM(type, name, expected) \
  type name;                                              \
  if (auto const res = (expected); !res)                  \
  {                                                       \
    return std::unexpected{res.error()};                  \
  }                                                       \
  else                                                    \
  {                                                       \
    name = res.value();                                   \
  }
#define RETURN_OK() \
  return std::expected<void, XMLError> {}

    struct OnExitScope
    {
      std::function<void()> onExit;

      ~OnExitScope()
      {
        onExit();
      }
    };

    std::expected<size_t, XMLError> GetFileSize(std::string_view filepath)
    {
      std::error_code ec;
      size_t const size = std::filesystem::file_size(filepath, ec);

      if (ec)
      {
        return std::unexpected{XMLError{ErrorReason::CANNOT_FIND_FILE, std::format("Cannot get file size of {}", filepath)}};
      }

      return size;
    }

    std::pair<size_t, char> FindFirstChar(std::string_view const buffer, std::initializer_list<char> chars, size_t offset = 0, bool reverse = false)
    {
      size_t min = std::string_view::npos;
      char foundChar;

      for (char c : chars)
      {
        size_t const pos = reverse ? buffer.rfind(c, offset) : buffer.find(c, offset);
        if (pos < min)
        {
          foundChar = c;
          min = pos;
        }
      }

      return {min, foundChar};
    }

    // std::string_view TrimWhitespace(std::string_view str)
    // {
    //   size_t const strBegin{str.find_first_not_of(" \t")};
    //   if (strBegin == std::string_view::npos)
    //   {
    //     return "";
    //   }

    //   size_t const strEnd{str.find_last_not_of(" \t")};
    //   return str.substr(strBegin, strEnd - strBegin + 1);
    // }

    FORCE_INLINE std::expected<char, ErrorReason> SafeAccess(std::string_view str, size_t index)
    {
      if (index >= str.size()) return std::unexpected{ErrorReason::PARSE_ERROR};
      return str[index];
    }

    FORCE_INLINE std::expected<size_t, ErrorReason> SafeFind(std::string_view str, char c, size_t offset = 0)
    {
      size_t const pos = str.find(c, offset);
      if (pos == std::string_view::npos) return std::unexpected{ErrorReason::PARSE_ERROR};
      return pos;
    }

    FORCE_INLINE std::expected<size_t, ErrorReason> SafeFind(std::string_view str, std::string_view toFind, size_t offset = 0)
    {
      size_t const pos = str.find(toFind, offset);
      if (pos == std::string_view::npos) return std::unexpected{ErrorReason::PARSE_ERROR};
      return pos;
    }

    FORCE_INLINE std::expected<size_t, ErrorReason> SafeFindList(std::string_view str, std::initializer_list<std::string_view> list, size_t offset = 0)
    {
      size_t pos = std::string_view::npos;
      for (std::string_view toFind : list)
      {
        size_t const found = str.find(toFind, offset);
        pos = std::min(found, pos);
      }
      if (pos == std::string_view::npos) return std::unexpected{ErrorReason::PARSE_ERROR};
      return pos;
    }

    FORCE_INLINE std::expected<std::string_view, ErrorReason> SafeGet(std::string_view str, size_t nr, size_t offset = 0)
    {
      if (nr >= str.size()) return std::unexpected{ErrorReason::PARSE_ERROR};
      return str.substr(offset, nr);
    }

    bool IsFullyWhiteSpace(std::string_view str)
    {
      for (char const c : str)
      {
        if (!std::isspace(c))
        {
          return false;
        }
      }

      return true;
    }
  }  // namespace

  XMLParser::~XMLParser()
  {
    if (m_buffer)
    {
      delete[] m_buffer;
      m_buffer = nullptr;
    }
  }

  std::expected<XMLDocument, XMLError> XMLParser::Parse(std::string_view filepath)
  {
    return GetFileSize(filepath).and_then(
        [this, filepath](size_t size)
        {
          m_bufferSize = size;
          m_buffer = new char[m_bufferSize]{};
          return ParseImpl(filepath);
        });
  }
  std::expected<XMLDocument, XMLError> XMLParser::Parse(std::string_view filepath, Buffer const& buffer)
  {
    return GetFileSize(filepath)
        .and_then(
            [filepath, &buffer, this](size_t size) -> std::expected<void, XMLError>
            {
              if (buffer.bufferSize < size)
              {
                return std::unexpected{XMLError{ErrorReason::BUFFER_TOO_SMALL, "Provided buffer is too small for filesize"}};
              }

              return {};
            })
        .and_then(
            [this, &buffer, filepath]()
            {
              m_bufferSize = buffer.bufferSize;
              m_buffer = buffer.buffer;
              return ParseImpl(filepath);
            });
  }

  std::expected<XMLDocument, XMLError> XMLParser::ParseImpl(std::string_view filepath)
  {
    {
      std::ifstream file{std::string(filepath)};
      if (!file.is_open())
      {
        return std::unexpected{XMLError{ErrorReason::CANNOT_OPEN_FILE, std::format("Could not open '{}' for reading", filepath)}};
      }

      file.read(m_buffer, m_bufferSize);
    }

    XMLDocument doc;
    if (auto const ret = ParseDocument(m_buffer, m_bufferSize, m_bufferPointer); !ret.has_value())
    {
      return std::unexpected{ret.error()};
    }

    for (size_t i{}; i < m_elements.size(); ++i)
    {
      doc.AddXMLElement(std::move(m_elements[i]), false);
    }

    return doc;
  }

  std::expected<void, XMLError> XMLParser::ParseDocument(std::string_view const buffer, size_t bufferSize, size_t& bufferPointer)
  {
    while (bufferPointer < bufferSize)
    {
      CHECK_EXPECTED(std::string_view, start, SafeGet(buffer, 2, bufferPointer), "EOF reached while parsing, file seems incomplete");
      if (start == "<!")
      {
        // [TODO]: Parse Comment
      }
      else if (start == "</")
      {
        if (m_elementStack.empty())
        {
          return std::unexpected{XMLError{ErrorReason::PARSE_ERROR, "End tag reached while no start tag was parsed"}};
        }

        CHECK_EXPECTED_VOID(ParseEndTag(buffer, bufferPointer));
      }
      else if (start[0] == '<')
      {
        CHECK_EXPECTED_VOID(ParseStartTag(buffer, bufferPointer));
      }
      else
      {
        if (IsFullyWhiteSpace(start))
        {
          bufferPointer += start.size();
          continue;
        }

        if (m_elementStack.empty())
        {
          return std::unexpected{XMLError{ErrorReason::PARSE_ERROR, "Trying to parse content when no start tag was parsed"}};
        }

        ParseContent(buffer, bufferPointer);
      }
    }

    RETURN_OK();
  }

  std::expected<void, XMLError> XMLParser::ParseStartTag(std::string_view const buffer, size_t& bufferPointer)
  {
    // Tags must start with <
    if (SafeAccess(buffer, bufferPointer) != '<')
    {
      return std::unexpected{XMLError{ErrorReason::PARSE_ERROR, "Expected <"}};
    }

    // Our 'rawTag' will look like <NAME [Attributes]
    CHECK_EXPECTED(size_t, pos, SafeFind(buffer, '>', bufferPointer + 1), "Could not find closing bracket of start tag");
    std::string_view const rawTag = buffer.substr(bufferPointer, pos - bufferPointer + 1);  // + 1 to capture closing bracket as well

    // Check for attributes ('=') or closing bracket
    std::pair<size_t, char> p = FindFirstChar(rawTag, {'=', '>'});
    if (p.first == std::string_view::npos)
    {
      return std::unexpected{XMLError{ErrorReason::PARSE_ERROR, "Start tag is not properly closed"}};
    }

    CHECK_EXPECTED(char, c, SafeAccess(buffer, 1), "Start tag is malformed");
    if (std::isspace(c))
    {
      return std::unexpected{XMLError{ErrorReason::PARSE_ERROR, "Start tag is malformed"}};
    }

    XMLTag tag;
    tag.name = rawTag.substr(1, FindFirstChar(rawTag, {'=', '>', ' ', '\t'}).first - 1);

    size_t attrOffset{};
    while (true)
    {
      size_t attrPos = rawTag.find('=', attrOffset);
      if (attrPos == std::string_view::npos)
      {
        break;
      }

      // find left hand part of attribute
      auto [leftHandPos, _c] = FindFirstChar(rawTag, {' ', '\t'}, attrPos, true);
      if (leftHandPos == std::string_view::npos)
      {
        return std::unexpected{XMLError{ErrorReason::PARSE_ERROR, std::format("Attribute in tag '{}' is malformed", tag.name)}};
      }

      // Find right hand part of attribute
      auto [rightHandPos, _c2] = FindFirstChar(rawTag, {' ', '\t', '\\', '>'}, attrPos);
      if (rightHandPos == std::string_view::npos)
      {
        return std::unexpected{XMLError{ErrorReason::PARSE_ERROR, std::format("Attribute in tag '{}' is malformed", tag.name)}};
      }

      std::string_view const key = rawTag.substr(leftHandPos, attrPos - leftHandPos);
      if (tag.attributes.contains(key))
      {
        return std::unexpected{XMLError{ErrorReason::PARSE_ERROR, std::format("Attribute key '{}' already present in tag '{}'", key, tag.name)}};
      }

      std::string_view const value = rawTag.substr(attrPos + 1, rightHandPos);
      tag.attributes.emplace(key, value);

      attrOffset = attrPos + 1;
    }

    if (SafeGet(rawTag, 2, rawTag.size() - 2) == "/>")
    {
      m_elements.push_back(XMLElement{.tag = std::move(tag), .children = {}, .content = ""});
    }
    else
    {
      m_elements.push_back(XMLElement{.tag = std::move(tag), .children = {}, .content = ""});
      m_elementStack.emplace(std::move(tag));
    }

    bufferPointer += rawTag.size();

    RETURN_OK();
  }

  std::expected<void, XMLError> XMLParser::ParseEndTag(std::string_view buffer, size_t& bufferPointer)
  {
    if (SafeAccess(buffer, bufferPointer) != '<')
    {
      return std::unexpected{XMLError{ErrorReason::PARSE_ERROR, "End tag must start with '</'"}};
    }

    // Our 'rawTag' will look like </NAME
    CHECK_EXPECTED(size_t, pos, SafeFind(buffer, '>', bufferPointer + 1), "End tag closing bracket could not be found");
    std::string_view const rawTag = buffer.substr(bufferPointer, pos - bufferPointer);

    if (SafeAccess(buffer, bufferPointer + 1) != '/')
    {
      return std::unexpected{XMLError{ErrorReason::PARSE_ERROR, "End tag must start with '</'"}};
    }

    bufferPointer += rawTag.size() + 1;  // + 1 because 'rawTag' does not have the closing bracket

    if (m_elementStack.top().tag.name != rawTag.substr(2))
    {
      return std::unexpected{XMLError{ErrorReason::PARSE_ERROR, std::format("No matching start tag found for end tag '{}'", rawTag.substr(2))}};
    }

    *(std::ranges::find_if(m_elements, [rawTag](XMLElement const& element) { return element.tag.name == rawTag.substr(2); })) = m_elementStack.top();
    m_elementStack.pop();

    RETURN_OK();
  }

  std::expected<void, XMLError> XMLParser::ParseContent(std::string_view buffer, size_t& bufferPointer)
  {
    // Search until the next node
    CHECK_EXPECTED(size_t, pos, SafeFind(buffer, '<', bufferPointer + 1), "No end tag found for content");

    std::string_view const content = buffer.substr(bufferPointer, pos - bufferPointer);
    m_elementStack.top().content = content;

    bufferPointer += content.size();

    RETURN_OK();
  }
}  // namespace fxml