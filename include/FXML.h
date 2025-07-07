#pragma once

#include <expected>
#include <stack>
#include <string>
#include <string_view>

#include "FXMLData.h"

namespace fxml
{
  enum class ErrorReason
  {
    PARSE_ERROR = 0,
    CANNOT_OPEN_FILE = 1,
    CANNOT_FIND_FILE = 2,
    BUFFER_TOO_SMALL = 3
  };

  class XMLError
  {
   private:
    std::string m_message;
    ErrorReason m_errorReason;

   public:
    XMLError(ErrorReason reason, std::string message)
      : m_message(message)
      , m_errorReason(reason)
    {
    }

    inline std::string const& what() const
    {
      return m_message;
    }

    inline ErrorReason reason() const
    {
      return m_errorReason;
    }
  };

  struct Buffer
  {
    char* buffer;
    size_t bufferSize;

    explicit Buffer(char* _buffer, size_t _bufferSize)
      : buffer(_buffer)
      , bufferSize(_bufferSize)
    {
    }
  };

  class XMLParser
  {
   private:
    char* m_buffer = nullptr;
    size_t m_bufferSize = 0;
    size_t m_bufferPointer = 0;
    std::stack<XMLElement> m_elementStack;
    std::vector<XMLElement> m_elements;

   public:
    ~XMLParser();

    // Parser will allocate buffer according to filesize
    std::expected<XMLDocument, XMLError> Parse(std::string_view filepath);

    // Parser will use a user-provided buffer
    std::expected<XMLDocument, XMLError> Parse(std::string_view filepath, Buffer const& buffer);

   private:
    std::expected<XMLDocument, XMLError> ParseImpl(std::string_view filepath);

    // =============================
    // ====== PARSE FUNCTIONS ======
    // =============================
    std::expected<void, XMLError> ParseDocument(std::string_view const buffer, size_t bufferSize, size_t& bufferPointer);
    std::expected<void, XMLError> ParseStartTag(std::string_view const buffer, size_t& bufferPointer);
    std::expected<void, XMLError> ParseEndTag(std::string_view buffer, size_t& bufferPointer);
    std::expected<void, XMLError> ParseContent(std::string_view buffer, size_t& bufferPointer);
    std::expected<void, XMLError> ParseComment(std::string_view buffer, size_t& bufferPointer);
  };
}  // namespace fxml