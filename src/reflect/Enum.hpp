#pragma once

#include <cstddef>
#include <iterator>
#include <string_view>

namespace cinder::reflect {

template <class E>
struct EnumNames;

constexpr char lowerAscii(char c) { return c >= 'A' && c <= 'Z' ? static_cast<char>(c + 32) : c; }

constexpr bool equalsIgnoreAscii(std::string_view a, std::string_view b) {
    if (a.size() != b.size()) return false;
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (lowerAscii(a[i]) != lowerAscii(b[i])) return false;
    }
    return true;
}

}

#define CINDER_ENUM_NAMES(Type, ...)                                       \
    namespace cinder::reflect {                                            \
    template <>                                                        \
    struct EnumNames<Type> {                                           \
        static constexpr std::string_view names[] = {__VA_ARGS__};     \
        static constexpr std::size_t count = std::size(names);         \
    };                                                                 \
    }
