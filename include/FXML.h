#pragma once

#include "FXMLData.h"

#include <expected>
#include <stack>
#include <string_view>

namespace fxml
{
	enum class ErrorReason
	{
		PARSE_ERROR = 0,
		CANNOT_OPEN_FILE = 1,
		CANNOT_FIND_FILE = 2,
		BUFFER_TOO_SMALL = 3
	};

	struct Buffer
	{
		char* buffer;
		uint32_t bufferSize;

		explicit Buffer(char* _buffer, uint32_t _bufferSize)
			: buffer(_buffer)
			, bufferSize(_bufferSize)
		{
		}
	};

	class XMLParser
	{
	private:
		char* m_buffer = nullptr;
		uint32_t m_bufferSize = 0;
		uint32_t m_bufferPointer = 0;
		std::stack<XMLElement> m_tags;

	public:
		~XMLParser();

		// Parser will allocate buffer according to filesize
		std::expected<XMLDocument, ErrorReason> Parse(std::string_view filepath);

		// Parser will use a user-provided buffer
		std::expected<XMLDocument, ErrorReason> Parse(std::string_view filepath, Buffer const& buffer);

	private:
		std::expected<XMLDocument, ErrorReason> ParseImpl(std::string_view filepath);

		// =============================
		// ====== PARSE FUNCTIONS ======
		// =============================
		std::expected<void, ErrorReason> ParseDocument(XMLDocument& document, std::string_view const buffer, uint32_t bufferSize, uint32_t& bufferPointer);
		std::expected<void, ErrorReason> ParseEndTag(XMLDocument& document, std::string_view buffer, uint32_t& bufferPointer);
		std::expected<void, ErrorReason> ParseContent(XMLDocument& document, std::string_view buffer, uint32_t& bufferPointer);
	};
} // namespace fxml