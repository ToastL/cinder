#include "reflect/Reflect.hpp"

namespace cinder::reflect {
namespace {

constexpr bool isUpperAscii(char c) { return c >= 'A' && c <= 'Z'; }

}

std::string deriveLabel(std::string_view field) {
    std::string out;
    if (field.empty()) return out;

    out.reserve(field.size() + 4);
    out.push_back(isUpperAscii(field[0]) ? field[0] : static_cast<char>(field[0] - 32));

    for (std::size_t i = 1; i < field.size(); ++i) {
        if (isUpperAscii(field[i]) && !isUpperAscii(field[i - 1])) out.push_back(' ');
        out.push_back(field[i]);
    }
    return out;
}

}
