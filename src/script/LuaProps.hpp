#pragma once

#include "scene/PropValue.hpp"

#include <lua.hpp>

#include <optional>

namespace cinder::script {

void pushProp(lua_State* state, const cinder::scene::PropValue& value);
void pushRec(lua_State* state, const cinder::scene::PropRec& values);

std::optional<cinder::scene::PropValue> readProp(lua_State* state, int index);
cinder::scene::PropRec readRec(lua_State* state, int index);

}
