#include "lua/LuaTable.hpp"

#include "lua/LuaSource.hpp"

#include <stdexcept>
#include <utility>

namespace cinder::lua {

LuaTable LuaTable::fromFile(const std::filesystem::path& path, const char* global) {
    lua_State* state = luaL_newstate();
    luaL_openlibs(state);

    const std::string source = readSource(path);
    if (luaL_dostring(state, source.c_str()) != LUA_OK) {
        const std::string message = lua_tostring(state, -1);
        lua_close(state);
        throw std::runtime_error(path.string() + ": " + message);
    }

    lua_getglobal(state, global);
    if (lua_istable(state, -1) == 0) {
        lua_close(state);
        throw std::runtime_error(path.string() + " must define a global table named `"
                                 + global + "`");
    }

    return LuaTable(state);
}

LuaTable::LuaTable(LuaTable&& other) noexcept : state_(std::exchange(other.state_, nullptr)) {}

std::string LuaTable::str(const char* key, const std::string& fallback) const {
    lua_getfield(state_, -1, key);
    const std::string value = lua_isstring(state_, -1) != 0 ? lua_tostring(state_, -1) : fallback;
    lua_pop(state_, 1);
    return value;
}

int LuaTable::num(const char* key, int fallback) const {
    lua_getfield(state_, -1, key);
    const int value = lua_isnumber(state_, -1) != 0
            ? static_cast<int>(lua_tonumber(state_, -1))
            : fallback;
    lua_pop(state_, 1);
    return value;
}

LuaTable::~LuaTable() {
    if (state_ != nullptr) lua_close(state_);
}

}
