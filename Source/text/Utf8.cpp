#include "text/Utf8.hpp"

namespace cinder::text {

namespace {

bool wordCharacter(char32_t codepoint) {
    if (codepoint >= 0x80) return true;
    return (codepoint >= U'a' && codepoint <= U'z') || (codepoint >= U'A' && codepoint <= U'Z')
            || (codepoint >= U'0' && codepoint <= U'9') || codepoint == U'_';
}

}

bool isContinuation(char byte) { return (static_cast<unsigned char>(byte) & 0xC0u) == 0x80u; }

std::size_t previousBoundary(std::string_view text, std::size_t offset) {
    if (offset == 0) return 0;
    if (offset > text.size()) return text.size();
    --offset;
    while (offset > 0 && isContinuation(text[offset])) --offset;
    return offset;
}

std::size_t nextBoundary(std::string_view text, std::size_t offset) {
    if (offset >= text.size()) return text.size();
    ++offset;
    while (offset < text.size() && isContinuation(text[offset])) ++offset;
    return offset;
}

std::size_t previousWord(std::string_view text, std::size_t offset) {
    offset = previousBoundary(text, offset);
    while (offset > 0 && !wordCharacter(decodeAt(text, offset))) offset = previousBoundary(text, offset);
    while (offset > 0 && wordCharacter(decodeAt(text, previousBoundary(text, offset)))) {
        offset = previousBoundary(text, offset);
    }
    return offset;
}

std::size_t nextWord(std::string_view text, std::size_t offset) {
    while (offset < text.size() && !wordCharacter(decodeAt(text, offset))) offset = nextBoundary(text, offset);
    while (offset < text.size() && wordCharacter(decodeAt(text, offset))) offset = nextBoundary(text, offset);
    return offset;
}

std::size_t characterCount(std::string_view text) {
    std::size_t count = 0;
    for (const char byte : text) {
        if (!isContinuation(byte)) ++count;
    }
    return count;
}

void appendUtf8(std::string& out, char32_t codepoint) {
    const auto value = static_cast<std::uint32_t>(codepoint);
    if (value < 0x80u) {
        out.push_back(static_cast<char>(value));
    } else if (value < 0x800u) {
        out.push_back(static_cast<char>(0xC0u | (value >> 6)));
        out.push_back(static_cast<char>(0x80u | (value & 0x3Fu)));
    } else if (value < 0x10000u) {
        out.push_back(static_cast<char>(0xE0u | (value >> 12)));
        out.push_back(static_cast<char>(0x80u | ((value >> 6) & 0x3Fu)));
        out.push_back(static_cast<char>(0x80u | (value & 0x3Fu)));
    } else if (value < 0x110000u) {
        out.push_back(static_cast<char>(0xF0u | (value >> 18)));
        out.push_back(static_cast<char>(0x80u | ((value >> 12) & 0x3Fu)));
        out.push_back(static_cast<char>(0x80u | ((value >> 6) & 0x3Fu)));
        out.push_back(static_cast<char>(0x80u | (value & 0x3Fu)));
    }
}

char32_t decodeAt(std::string_view text, std::size_t offset) {
    if (offset >= text.size()) return 0;
    const auto lead = static_cast<unsigned char>(text[offset]);
    int length = 1;
    std::uint32_t value = lead;
    if (lead >= 0xF0u) {
        length = 4;
        value = lead & 0x07u;
    } else if (lead >= 0xE0u) {
        length = 3;
        value = lead & 0x0Fu;
    } else if (lead >= 0xC0u) {
        length = 2;
        value = lead & 0x1Fu;
    }
    for (int i = 1; i < length; ++i) {
        if (offset + static_cast<std::size_t>(i) >= text.size()) return 0xFFFD;
        value = (value << 6) | (static_cast<unsigned char>(text[offset + static_cast<std::size_t>(i)]) & 0x3Fu);
    }
    return static_cast<char32_t>(value);
}

}
