#pragma once

#include <lua.hpp>

namespace cinder::lua {

class LuaApi {
public:
    LuaApi(lua_State* state, void* context);

    void bind(const char* name, lua_CFunction fn);
    void bind(const char* name, lua_CFunction fn, void* context);
    void install(const char* global);

    static float optFloat(lua_State* state, int index, float fallback);
    static int optInt(lua_State* state, int index, int fallback);

    template <class T>
    static T* context(lua_State* state) {
        return static_cast<T*>(lua_touserdata(state, lua_upvalueindex(1)));
    }

private:
    lua_State* state_;
    void* context_;
    int tableIndex_;
};

}
