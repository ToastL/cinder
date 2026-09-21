#pragma once

#include <lua.hpp>

#include <string_view>

namespace cinder::lua {

class StackRestore {
public:
    explicit StackRestore(lua_State* state) : state_(state), top_(lua_gettop(state)) {}
    ~StackRestore() { lua_settop(state_, top_); }

    StackRestore(const StackRestore&) = delete;
    StackRestore& operator=(const StackRestore&) = delete;

private:
    lua_State* state_;
    int top_;
};

bool pushFunction(lua_State* state, const char* name);
bool protectedCall(lua_State* state, int arguments, int results, const char* context = nullptr);

bool runChunk(lua_State* state, std::string_view source, const char* chunkname);

void callGlobal(lua_State* state, const char* name);
void callGlobal(lua_State* state, const char* name, double argument);
void callMethod(lua_State* state, int ref, const char* name);
void callMethod(lua_State* state, int ref, const char* name, double argument);

}
