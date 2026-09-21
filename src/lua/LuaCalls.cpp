#include "lua/LuaCalls.hpp"

#include "platform/Log.hpp"

namespace cinder::lua {
namespace {

void global(lua_State* state, const char* name, const double* argument) {
    StackRestore stack(state);

    if (!pushFunction(state, name)) return;

    int count = 0;
    if (argument != nullptr) {
        lua_pushnumber(state, *argument);
        count = 1;
    }

    protectedCall(state, count, 0, name);
}

void method(lua_State* state, int ref, const char* name, const double* argument) {
    StackRestore stack(state);

    lua_rawgeti(state, LUA_REGISTRYINDEX, ref);
    lua_getfield(state, -1, name);
    if (lua_isfunction(state, -1) == 0) return;

    lua_pushvalue(state, -2);
    int count = 1;
    if (argument != nullptr) {
        lua_pushnumber(state, *argument);
        count = 2;
    }

    protectedCall(state, count, 0, name);
}

}

bool pushFunction(lua_State* state, const char* name) {
    lua_getglobal(state, name);
    if (lua_isfunction(state, -1)) return true;
    lua_pop(state, 1);
    return false;
}

bool protectedCall(lua_State* state, int arguments, int results, const char* context) {
    if (lua_pcall(state, arguments, results, 0) == LUA_OK) return true;
    const char* message = lua_tostring(state, -1);
    if (context != nullptr) {
        cinder::platform::logError("[lua] %s: %s\n", context,
                                   message != nullptr ? message : "unknown error");
    } else {
        cinder::platform::logError("[lua] %s\n", message != nullptr ? message : "unknown error");
    }
    lua_pop(state, 1);
    return false;
}

bool runChunk(lua_State* state, std::string_view source, const char* chunkname) {
    if (luaL_loadbuffer(state, source.data(), source.size(), chunkname) != LUA_OK) return false;
    return lua_pcall(state, 0, 0, 0) == LUA_OK;
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
