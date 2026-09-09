#include "lua/LuaCalls.hpp"

#include <cstdio>

namespace cinder::lua {
namespace {

void report(const char* name, lua_State* state) {
    const char* message = lua_tostring(state, -1);
    std::fprintf(stderr, "[lua] %s: %s\n", name, message != nullptr ? message : "unknown error");
    lua_pop(state, 1);
}

void global(lua_State* state, const char* name, const double* argument) {
    const int top = lua_gettop(state);

    lua_getglobal(state, name);
    if (lua_isfunction(state, -1) == 0) {
        lua_settop(state, top);
        return;
    }

    int count = 0;
    if (argument != nullptr) {
        lua_pushnumber(state, *argument);
        count = 1;
    }

    if (lua_pcall(state, count, 0, 0) != LUA_OK) report(name, state);
    lua_settop(state, top);
}

void method(lua_State* state, int ref, const char* name, const double* argument) {
    const int top = lua_gettop(state);

    lua_rawgeti(state, LUA_REGISTRYINDEX, ref);
    lua_getfield(state, -1, name);
    if (lua_isfunction(state, -1) == 0) {
        lua_settop(state, top);
        return;
    }

    lua_pushvalue(state, -2);
    int count = 1;
    if (argument != nullptr) {
        lua_pushnumber(state, *argument);
        count = 2;
    }

    if (lua_pcall(state, count, 0, 0) != LUA_OK) report(name, state);
    lua_settop(state, top);
}

}

void callGlobal(lua_State* state, const char* name) { global(state, name, nullptr); }

void callGlobal(lua_State* state, const char* name, double argument) {
    global(state, name, &argument);
}

void callMethod(lua_State* state, int ref, const char* name) { method(state, ref, name, nullptr); }

void callMethod(lua_State* state, int ref, const char* name, double argument) {
    method(state, ref, name, &argument);
}

}
