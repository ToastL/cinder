#pragma once

#include "reflect/PropDef.hpp"
#include "scene/PropValue.hpp"

#include <optional>
#include <string_view>

namespace cinder::dev::propertyWidgets {

constexpr float LABEL_WEIGHT = 0.4f;

void pushId(std::string_view id);
bool beginRows(const char* id);
void row(std::string_view label);
bool propRows(const cinder::reflect::PropList& defs, void* target);
std::optional<cinder::scene::PropValue> editAttribute(const cinder::scene::PropValue& value);

}
