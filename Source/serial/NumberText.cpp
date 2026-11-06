#include "serial/NumberText.hpp"

#include "platform/Log.hpp"

#include <charconv>
#include <cmath>

namespace cinder::serial {

std::string integerText(std::int64_t value) {
    char buffer[32];
    auto [end, error] = std::to_chars(buffer, buffer + sizeof(buffer), value);
    return std::string(buffer, end);
}

std::string realText(double value) {
    char buffer[40];
    auto [end, error] = std::to_chars(buffer, buffer + sizeof(buffer), value);
    return std::string(buffer, end);
}

std::string number(float value) {
    if (!std::isfinite(value)) {
        cinder::platform::logError("[serial] non-finite value written as 0\n");
        return "0";
    }

    if (value == std::floor(value) && std::fabs(value) < static_cast<float>(1 << 24)) {
        char buffer[16];
        auto [end, error] = std::to_chars(buffer, buffer + sizeof(buffer),
                                          static_cast<int>(value));
        return std::string(buffer, end);
    }

    char buffer[32];
    auto [end, error] = std::to_chars(buffer, buffer + sizeof(buffer), value);
    return std::string(buffer, end);
}

std::string vectorText(std::span<const float> values) {
    std::string joined;
    for (float value : values) {
        if (!joined.empty()) joined.push_back(' ');
        joined += number(value);
    }
    return joined;
}

}
