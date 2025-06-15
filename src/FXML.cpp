#include "FXML.h"
#include "FXMLData.h"

#include <array>
#include <fstream>
#include <filesystem>
#include <limits>
#include <ranges>

namespace fxml
{
	namespace
	{
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

		std::expected<void, ErrorReason> ParseTag(XMLDocument& document, std::string_view const buffer, uint32_t bufferSize, uint32_t& bufferPointer)
		{
			if (buffer[bufferPointer] != '<')
			{
				return std::unexpected{ ErrorReason::PARSE_ERROR };
			}

			// Our 'rawTag' will look like <NAME [Attributes]
			std::string_view const rawTag = buffer.substr(bufferPointer, buffer.find('>', bufferPointer + 1));
			auto const [pos, c] = FindFirstChar(rawTag, {' ', '>', '\\'});
			if (pos == std::string_view::npos)
			{
				return std::unexpected{ ErrorReason::PARSE_ERROR };
			}

			XMLTag tag;
			tag.name = rawTag.substr(1, pos);

			document.AddTag(std::move(tag));

			return {};
		}

		std::expected<void, ErrorReason> ParseDocument(XMLDocument& document, std::string_view const buffer, uint32_t bufferSize, uint32_t& bufferPointer)
		{
			return ParseTag(document, buffer, bufferSize, bufferPointer);
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
} // namespace fxml