#include "FXML.h"
#include "FXMLData.h"

#include <array>
#include <format>
#include <fstream>
#include <filesystem>
#include <functional>
#include <limits>
#include <ranges>

#if  defined(__GNUC__) || defined(__GNUG__)
#define FORCE_INLINE __attribute__((always_inline)) inline
#elif defined(_MSC_VER)
#define FORCE_INLINE __forceinline
#endif

namespace fxml
{
	namespace
	{
		#define CHECK_EXPECTED_VOID(expected) if (auto const res = (expected); !res) { return std::unexpected{ res.error() }; }
		#define CHECK_EXPECTED(type, name, expected, errorMsg) type name; if (auto const res = (expected); !res) { return std::unexpected{ XMLError{res.error(), errorMsg} }; } else { name = res.value(); }
		#define CHECK_EXPECTED_NO_TRANSFORM(type, name, expected) type name; if (auto const res = (expected); !res) { return std::unexpected{ res.error() }; } else { name = res.value(); }
		#define RETURN_OK() return std::expected<void, XMLError>{}

		struct OnExitScope
		{
			std::function<void()> onExit;

			~OnExitScope()
			{
				onExit();
			}
		};

		std::expected<uint32_t, XMLError> GetFileSize(std::string_view filepath)
		{
			std::error_code ec;
			uint32_t const size = static_cast<uint32_t>(std::filesystem::file_size(filepath, ec));

			if (ec)
			{
				return std::unexpected{XMLError{ErrorReason::CANNOT_FIND_FILE, std::format("Cannot get file size of {}", filepath)}};
			}

			return size;
		}

		std::pair<uint32_t, char> FindFirstChar(std::string_view const buffer, std::initializer_list<char> chars)
		{
			uint32_t min = static_cast<uint32_t>(std::string_view::npos);
			char foundChar;

			for (char c : chars)
			{
				if (uint32_t const pos = static_cast<uint32_t>(buffer.find(c)); pos < min)
				{
					foundChar = c;
					min = pos;
				}
			}

			return { min, foundChar };
		}

		std::string_view TrimWhitespace(std::string_view str)
		{
			size_t const strBegin{ str.find_first_not_of(" \t") };
			if (strBegin == std::string_view::npos)
			{
				return "";
			}

			size_t const strEnd{ str.find_last_not_of(" \t") };
			return str.substr(strBegin, strEnd - strBegin + 1);
		}

		FORCE_INLINE std::expected<char, ErrorReason> SafeAccess(std::string_view str, uint32_t index)
		{
			if (index >= str.size()) return std::unexpected{ ErrorReason::PARSE_ERROR };
			return str[index];
		}

		FORCE_INLINE std::expected<uint32_t, ErrorReason> SafeFind(std::string_view str, char c, uint32_t offset = 0)
		{
			size_t const pos = str.find(c, offset);
			if (pos == std::string_view::npos) return std::unexpected{ ErrorReason::PARSE_ERROR };
			return static_cast<uint32_t>(pos);
		}

		FORCE_INLINE std::expected<uint32_t, ErrorReason> SafeFind(std::string_view str, std::string_view toFind, uint32_t offset = 0)
		{
			size_t const pos = str.find(toFind, offset);
			if (pos == std::string_view::npos) return std::unexpected{ ErrorReason::PARSE_ERROR };
			return static_cast<uint32_t>(pos);
		}

		FORCE_INLINE std::expected<uint32_t, ErrorReason> SafeFindList(std::string_view str, std::initializer_list<std::string_view> list, uint32_t offset = 0)
		{
			size_t pos = std::string_view::npos;
			for (std::string_view toFind : list)
			{
				size_t const found = str.find(toFind, offset);
				pos = std::min(found, pos);
			}
			if (pos == std::string_view::npos) return std::unexpected{ ErrorReason::PARSE_ERROR };
			return static_cast<uint32_t>(pos);
		}

		FORCE_INLINE std::expected<std::string_view, ErrorReason> SafeGet(std::string_view str, uint32_t nr)
		{
			if (nr >= str.size()) return std::unexpected{ ErrorReason::PARSE_ERROR };
			return str.substr(0, nr);
		}

		FORCE_INLINE std::expected<std::string_view, ErrorReason> SafeGet(std::string_view str, uint32_t nr, uint32_t offset)
		{
			if (nr >= str.size()) return std::unexpected{ ErrorReason::PARSE_ERROR };
			return str.substr(offset, nr);
		}

		bool IsFullyWhiteSpace(std::string_view str)
		{
			for (const char c : str)
			{
				if (!std::isspace(c))
				{
					return false;
				}
			}

			return true;
		}

		std::expected<XMLTag, XMLError> ParseStartTag(std::string_view const buffer, uint32_t& bufferPointer)
		{
			// Tags must start with <
			if (SafeAccess(buffer, bufferPointer) != '<')
			{
				return std::unexpected{ XMLError{ErrorReason::PARSE_ERROR, "Expected <"} };
			}

			// Our 'rawTag' will look like <NAME [Attributes]
			CHECK_EXPECTED(uint32_t, pos, SafeFind(buffer, '>', bufferPointer + 1), "Could not find closing bracket of start tag");
			std::string_view const rawTag = buffer.substr(bufferPointer, pos - bufferPointer);

			// Check for attributes ('='), closing bracket, or an empty-element tag
			std::pair<uint32_t, char> p = FindFirstChar(rawTag, {'=', '>', '\\'});
			if (p.first == std::string_view::npos)
			{
				return std::unexpected{ XMLError{ErrorReason::PARSE_ERROR, "Start tag is not properly closed"} };
			}

			XMLTag tag;
			tag.name = rawTag.substr(1, p.first);

			bufferPointer += static_cast<uint32_t>(rawTag.size()) + 1; // + 1 because 'rawTag' does not have the closing bracket

			return tag;
		}
	} // namespace

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
		return GetFileSize(filepath).and_then([this, filepath](uint32_t size)
			{
				m_bufferSize = size;
				m_buffer = new char[m_bufferSize] {};
				return ParseImpl(filepath);
			});
	}
	std::expected<XMLDocument, XMLError> XMLParser::Parse(std::string_view filepath, Buffer const& buffer)
	{
		return GetFileSize(filepath).and_then([filepath, &buffer, this](uint32_t size) -> std::expected<void, XMLError>
			{
				if (buffer.bufferSize < size)
				{
					return std::unexpected{ XMLError{ErrorReason::BUFFER_TOO_SMALL, "Provided buffer is too small for filesize"} };
				}

				return {};
			}).and_then([this, &buffer, filepath]()
				{
					m_bufferSize = buffer.bufferSize;
					m_buffer = buffer.buffer;
					return ParseImpl(filepath);
				});
	}

	std::expected<XMLDocument, XMLError> XMLParser::ParseImpl(std::string_view filepath)
	{
		{
			std::ifstream file{ std::string(filepath) };
			if (!file.is_open())
			{
				return std::unexpected{ XMLError{ErrorReason::CANNOT_OPEN_FILE, std::format("Could not open '{}' for reading", filepath)} };
			}

			file.read(m_buffer, m_bufferSize);
		}

		XMLDocument doc;
		if (const auto ret = ParseDocument(doc, m_buffer, m_bufferSize, m_bufferPointer); !ret.has_value())
		{
			return std::unexpected{ ret.error() };
		}

		return doc;
	}

	std::expected<void, XMLError> XMLParser::ParseDocument(XMLDocument& document, std::string_view const buffer, uint32_t bufferSize, uint32_t& bufferPointer)
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
				if (m_tags.empty())
				{
					return std::unexpected{ XMLError{ErrorReason::PARSE_ERROR, "End tag reached while no start tag was parsed"} };
				}

				CHECK_EXPECTED_VOID(ParseEndTag(document, buffer, bufferPointer));
			}
			else if (start[0] == '<')
			{
				CHECK_EXPECTED_NO_TRANSFORM(XMLTag, startTag, ParseStartTag(buffer, bufferPointer));
				m_tags.emplace(startTag);
			}
			else
			{
				if (IsFullyWhiteSpace(start))
				{
					bufferPointer += static_cast<uint32_t>(start.size());
					continue;
				}

				if (m_tags.empty())
				{
					return std::unexpected{ XMLError{ErrorReason::PARSE_ERROR, "Trying to parse content when no start tag was parsed"} };
				}

				ParseContent(document, buffer, bufferPointer);
			}
		}

		RETURN_OK();
	}

	std::expected<void, XMLError> XMLParser::ParseEndTag(XMLDocument& document, std::string_view buffer, uint32_t& bufferPointer)
	{
		if (SafeAccess(buffer, bufferPointer) != '<')
		{
			return std::unexpected{ XMLError{ErrorReason::PARSE_ERROR, "End tag must start with '</'"} };
		}

		// Our 'rawTag' will look like </NAME
		CHECK_EXPECTED(uint32_t, pos, SafeFind(buffer, '>', bufferPointer + 1), "End tag closing bracket could not be found");
		std::string_view const rawTag = buffer.substr(bufferPointer, pos - bufferPointer);

		if (SafeAccess(buffer, bufferPointer + 1) != '/')
		{
			return std::unexpected{ XMLError{ErrorReason::PARSE_ERROR, "End tag must start with '</'"} };
		}

		bufferPointer += static_cast<uint32_t>(rawTag.size()) + 1; // + 1 because 'rawTag' does not have the closing bracket

		if (m_tags.top().tag.name != rawTag.substr(2))
		{
			return std::unexpected{ XMLError{ErrorReason::PARSE_ERROR, std::format("No matching start tag found for end tag '{}'", rawTag.substr(2))} };
		}

		document.AddXMLElement(std::move(m_tags.top()));
		m_tags.pop();

		RETURN_OK();
	}

	std::expected<void, XMLError> XMLParser::ParseContent(XMLDocument& document, std::string_view buffer, uint32_t& bufferPointer)
	{
		// Search until the next node
		CHECK_EXPECTED(uint32_t, pos, SafeFind(buffer, '<', bufferPointer + 1), "No end tag found for content");

		std::string_view const content = buffer.substr(bufferPointer, pos - bufferPointer);
		m_tags.top().content = content;

		bufferPointer += static_cast<uint32_t>(content.size());

		RETURN_OK();
	}
} // namespace fxml