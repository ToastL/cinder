#pragma once

#include <cstdint>
#include <span>
#include <string>

namespace cinder::serial {

std::string integerText(std::int64_t value);
std::string realText(double value);
std::string number(float value);
std::string vectorText(std::span<const float> values);

}
