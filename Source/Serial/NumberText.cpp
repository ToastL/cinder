#include "Serial/NumberText.hpp"

#include "Platform/Log.hpp"

#include <charconv>
#include <cmath>

namespace cinder::Serial {

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

std::string floatText(float value) {
    if (!std::isfinite(value)) {
        cinder::Platform::logError("[serial] non-finite value written as 0\n");
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
        joined += floatText(value);
    }
    return joined;
}

namespace {

template <class T>
std::optional<T> parsed(std::string_view text) {
    T out{};
    const char* end = text.data() + text.size();
    auto [at, error] = std::from_chars(text.data(), end, out);
    if (error != std::errc{} || at != end || text.empty()) return std::nullopt;
    return out;
}

}

std::optional<std::int64_t> parseInteger(std::string_view text) { return parsed<std::int64_t>(text); }
std::optional<double> parseReal(std::string_view text) { return parsed<double>(text); }
std::optional<float> parseFloat(std::string_view text) { return parsed<float>(text); }

}
