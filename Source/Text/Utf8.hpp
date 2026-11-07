#pragma once

#include "Platform/Utf8.hpp"

#include <cstddef>
#include <string>
#include <string_view>

namespace cinder::Text {

bool isContinuation(char byte);
std::size_t previousBoundary(std::string_view text, std::size_t offset);
std::size_t nextBoundary(std::string_view text, std::size_t offset);
std::size_t previousWord(std::string_view text, std::size_t offset);
std::size_t nextWord(std::string_view text, std::size_t offset);
std::size_t characterCount(std::string_view text);

using cinder::Platform::appendUtf8;
char32_t decodeAt(std::string_view text, std::size_t offset);

}
