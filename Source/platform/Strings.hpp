#pragma once

#include <string_view>

namespace cinder::platform {

constexpr bool isBlank(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v'; }

constexpr std::string_view trimmed(std::string_view text) {
    while (!text.empty() && isBlank(text.front())) text.remove_prefix(1);
    while (!text.empty() && isBlank(text.back())) text.remove_suffix(1);
    return text;
}

}
