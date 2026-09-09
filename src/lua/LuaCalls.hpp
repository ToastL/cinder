#pragma once

#include <lua.hpp>

namespace cinder::lua {

void callGlobal(lua_State* state, const char* name);
void callGlobal(lua_State* state, const char* name, double argument);
void callMethod(lua_State* state, int ref, const char* name);
void callMethod(lua_State* state, int ref, const char* name, double argument);

}
