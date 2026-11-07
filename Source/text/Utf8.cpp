#include "text/Utf8.hpp"

#include "platform/Utf8.hpp"

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

char32_t decodeAt(std::string_view text, std::size_t offset) { return cinder::platform::decodeUtf8(text, offset); }

}
