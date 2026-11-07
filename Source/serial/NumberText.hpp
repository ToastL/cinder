#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace cinder::serial {

std::string integerText(std::int64_t value);
std::string realText(double value);
std::string floatText(float value);
std::string vectorText(std::span<const float> values);

std::optional<std::int64_t> parseInteger(std::string_view text);
std::optional<double> parseReal(std::string_view text);
std::optional<float> parseFloat(std::string_view text);

}
