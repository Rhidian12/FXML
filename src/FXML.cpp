#include "FXML.h"
#include "FXMLData.h"

#include <array>
#include <fstream>
#include <filesystem>
#include <functional>
#include <limits>
#include <ranges>

namespace fxml
{
	namespace
	{
		#define CHECK_EXPECTED_VOID(expected) if (auto const res = (expected); !res) { return std::unexpected{ res.error() }; }
		#define CHECK_EXPECTED(type, name, expected) type name; if (auto const res = (expected); !res) { return std::unexpected{ res.error() }; } else { name = res.value(); }
		#define RETURN_OK() return std::expected<void, ErrorReason>{}

		struct OnExitScope
		{
			std::function<void()> onExit;

			~OnExitScope()
			{
				onExit();
			}
		};

		std::expected<uint32_t, ErrorReason> GetFileSize(std::string_view filepath)
		{
			std::error_code ec;
			uint32_t const size = static_cast<uint32_t>(std::filesystem::file_size(filepath, ec));

			if (ec)
			{
				return std::unexpected{ ErrorReason::CANNOT_FIND_FILE };
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

		__forceinline std::expected<char, ErrorReason> SafeAccess(std::string_view str, uint32_t index)
		{
			if (index >= str.size()) return std::unexpected{ ErrorReason::PARSE_ERROR };
			return str[index];
		}

		__forceinline std::expected<uint32_t, ErrorReason> SafeFind(std::string_view str, char c, uint32_t offset = 0)
		{
			size_t const pos = str.find(c, offset);
			if (pos == std::string_view::npos) return std::unexpected{ ErrorReason::PARSE_ERROR };
			return static_cast<uint32_t>(pos);
		}

		__forceinline std::expected<uint32_t, ErrorReason> SafeFind(std::string_view str, std::string_view toFind, uint32_t offset = 0)
		{
			size_t const pos = str.find(toFind, offset);
			if (pos == std::string_view::npos) return std::unexpected{ ErrorReason::PARSE_ERROR };
			return static_cast<uint32_t>(pos);
		}

		__forceinline std::expected<uint32_t, ErrorReason> SafeFindList(std::string_view str, std::initializer_list<std::string_view> list, uint32_t offset = 0)
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

		__forceinline std::expected<std::string_view, ErrorReason> SafeGet(std::string_view str, uint32_t nr)
		{
			if (nr >= str.size()) return std::unexpected{ ErrorReason::PARSE_ERROR };
			return str.substr(0, nr);
		}

		std::expected<XMLTag, ErrorReason> ParseStartTag(XMLDocument& document, std::string_view const buffer, uint32_t& bufferPointer)
		{
			// Tags must start with <
			if (SafeAccess(buffer, bufferPointer) != '<')
			{
				return std::unexpected{ ErrorReason::PARSE_ERROR };
			}

			// Our 'rawTag' will look like <NAME [Attributes]
			CHECK_EXPECTED(uint32_t, pos, SafeFind(buffer, '>', bufferPointer + 1));
			std::string_view const rawTag = buffer.substr(bufferPointer, pos);
			std::pair<uint32_t, char> p = FindFirstChar(rawTag, {' ', '>', '\\'});
			if (p.first == std::string_view::npos)
			{
				return std::unexpected{ ErrorReason::PARSE_ERROR };
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

	std::expected<XMLDocument, ErrorReason> XMLParser::Parse(std::string_view filepath)
	{
		return GetFileSize(filepath).and_then([this, filepath](uint32_t size)
			{
				m_bufferSize = size;
				m_buffer = new char[m_bufferSize] {};
				return ParseImpl(filepath);
			});
	}
	std::expected<XMLDocument, ErrorReason> XMLParser::Parse(std::string_view filepath, Buffer const& buffer)
	{
		return GetFileSize(filepath).and_then([filepath, &buffer, this](uint32_t size) -> std::expected<void, ErrorReason>
			{
				if (buffer.bufferSize < size)
				{
					return std::unexpected{ ErrorReason::BUFFER_TOO_SMALL };
				}

				return {};
			}).and_then([this, &buffer, filepath]()
				{
					m_bufferSize = buffer.bufferSize;
					m_buffer = buffer.buffer;
					return ParseImpl(filepath);
				});
	}

	std::expected<XMLDocument, ErrorReason> XMLParser::ParseImpl(std::string_view filepath)
	{
		{
			std::ifstream file{ std::string(filepath) };
			if (!file.is_open())
			{
				return std::unexpected{ ErrorReason::CANNOT_OPEN_FILE };
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

	std::expected<void, ErrorReason> XMLParser::ParseDocument(XMLDocument& document, std::string_view const buffer, uint32_t bufferSize, uint32_t& bufferPointer)
	{
		while (bufferPointer < bufferSize)
		{
			CHECK_EXPECTED(std::string_view, start, SafeGet(buffer, 2));
			if (start == "<!")
			{
				// [TODO]: Parse Comment
			}
			else if (start == "</")
			{
				if (m_tags.empty())
				{
					return std::unexpected{ ErrorReason::PARSE_ERROR };
				}

				CHECK_EXPECTED_VOID(ParseEndTag(document, buffer, bufferPointer));
			}
			else if (start[0] == '<')
			{
				CHECK_EXPECTED(XMLTag, startTag, ParseStartTag(document, buffer, bufferPointer));
				m_tags.emplace(startTag);
			}
			else
			{
				if (m_tags.empty())
				{
					return std::unexpected{ ErrorReason::PARSE_ERROR };
				}

				ParseContent(document, buffer, bufferPointer);
			}
		}

		RETURN_OK();
	}

	std::expected<void, ErrorReason> XMLParser::ParseEndTag(XMLDocument& document, std::string_view buffer, uint32_t& bufferPointer)
	{
		if (SafeAccess(buffer, bufferPointer) != '<')
		{
			return std::unexpected{ ErrorReason::PARSE_ERROR };
		}

		// Our 'rawTag' will look like </NAME
		CHECK_EXPECTED(uint32_t, pos, SafeFind(buffer, '>', bufferPointer + 1));
		std::string_view const rawTag = buffer.substr(bufferPointer, pos);

		if (SafeAccess(buffer, bufferPointer + 1) != '/')
		{
			return std::unexpected{ ErrorReason::PARSE_ERROR };
		}

		bufferPointer += static_cast<uint32_t>(rawTag.size()) + 1; // + 1 because 'rawTag' does not have the closing bracket

		if (m_tags.top().tag.name != rawTag.substr(2))
		{
			return std::unexpected{ ErrorReason::PARSE_ERROR };
		}

		document.AddXMLElement(std::move(m_tags.top()));
		m_tags.pop();

		RETURN_OK();
	}

	std::expected<void, ErrorReason> ParseContent(XMLDocument& document, std::string_view buffer, uint32_t& bufferPointer)
	{
		// Search until the next node
		CHECK_EXPECTED(uint32_t, pos, SafeFind(buffer, '<', bufferPointer + 1));

	}
} // namespace fxml