#include "scene/Attributes.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

namespace cinder::scene {
namespace {

constexpr std::size_t MAX_NAME = 100;

constexpr bool isNameChar(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
}

bool isNumber(const PropValue& value) { return value.is<std::int64_t>() || value.is<double>(); }

double numberOf(const PropValue& value) {
    return value.is<double>() ? value.as<double>() : static_cast<double>(value.as<std::int64_t>());
}

}

bool isAttributeName(std::string_view name) {
    if (name.empty() || name.size() > MAX_NAME) return false;
    for (char c : name) {
        if (!isNameChar(c)) return false;
    }
    return true;
}

bool isAttributeValue(const PropValue& value) {
    if (isNumber(value) || value.is<std::string>() || value.is<bool>()) return true;
    if (!value.is<PropSeq>()) return false;

    const PropSeq& items = value.as<PropSeq>();
    if (items.size() < 2 || items.size() > 4) return false;
    for (const PropValue& item : items) {
        if (!isNumber(item)) return false;
    }
    return true;
}

bool sameAttribute(const PropValue& a, const PropValue& b) {
    if (isNumber(a) && isNumber(b)) return numberOf(a) == numberOf(b);
    if (a.is<std::string>() && b.is<std::string>()) return a.as<std::string>() == b.as<std::string>();
    if (a.is<bool>() && b.is<bool>()) return a.as<bool>() == b.as<bool>();
    if (!a.is<PropSeq>() || !b.is<PropSeq>()) return false;

    const PropSeq& left = a.as<PropSeq>();
    const PropSeq& right = b.as<PropSeq>();
    if (left.size() != right.size()) return false;
    for (std::size_t i = 0; i < left.size(); ++i) {
        if (!sameAttribute(left[i], right[i])) return false;
    }
    return true;
}

}
