#include "lua/LuaApi.hpp"

#include <stdexcept>
#include <string>

namespace cinder::lua {

LuaApi::LuaApi(lua_State* state, void* context) : state_(state), context_(context) {
    lua_createtable(state, 0, 16);
    tableIndex_ = lua_gettop(state);
}

void LuaApi::bind(const char* name, lua_CFunction fn) { bind(name, fn, context_); }

void LuaApi::bind(const char* name, lua_CFunction fn, void* context) {
    if (lua_gettop(state_) != tableIndex_) {
        throw std::runtime_error(std::string("lua stack unbalanced while binding ") + name
                                 + ": expected top " + std::to_string(tableIndex_)
                                 + ", got " + std::to_string(lua_gettop(state_)));
    }
    lua_pushlightuserdata(state_, context);
    lua_pushcclosure(state_, fn, 1);
    lua_setfield(state_, -2, name);
}

void LuaApi::install(const char* global) { lua_setglobal(state_, global); }

float LuaApi::optFloat(lua_State* state, int index, float fallback) {
    if (lua_isnoneornil(state, index)) return fallback;
    return static_cast<float>(lua_tonumber(state, index));
}

int LuaApi::optInt(lua_State* state, int index, int fallback) {
    if (lua_isnoneornil(state, index)) return fallback;
    return static_cast<int>(lua_tointeger(state, index));
}

}
