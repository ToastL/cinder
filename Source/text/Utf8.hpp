#pragma once

#include "platform/Utf8.hpp"

#include <cstddef>
#include <string>
#include <string_view>

namespace cinder::text {

bool isContinuation(char byte);
std::size_t previousBoundary(std::string_view text, std::size_t offset);
std::size_t nextBoundary(std::string_view text, std::size_t offset);
std::size_t previousWord(std::string_view text, std::size_t offset);
std::size_t nextWord(std::string_view text, std::size_t offset);
std::size_t characterCount(std::string_view text);

using cinder::platform::appendUtf8;
char32_t decodeAt(std::string_view text, std::size_t offset);

}
