#pragma once

#include <cstddef>
#include <string>
#include <string_view>

namespace cinder::Platform {

void appendUtf8(std::string& out, char32_t codepoint);
char32_t decodeUtf8(std::string_view text, std::size_t& offset);

}
