#pragma once

#include "Scene/PropValue.hpp"

#include <string_view>

namespace cinder::Scene {

bool isAttributeName(std::string_view name);
bool isAttributeValue(const PropValue& value);
bool sameAttribute(const PropValue& a, const PropValue& b);

}
