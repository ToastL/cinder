#pragma once

#include <lua.hpp>

#include <string_view>

namespace cinder::lua {

bool runChunk(lua_State* state, std::string_view source, const char* chunkname);

void callGlobal(lua_State* state, const char* name);
void callGlobal(lua_State* state, const char* name, double argument);
void callMethod(lua_State* state, int ref, const char* name);
void callMethod(lua_State* state, int ref, const char* name, double argument);

}
