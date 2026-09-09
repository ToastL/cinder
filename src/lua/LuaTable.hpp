#pragma once

#include <lua.hpp>

#include <filesystem>
#include <string>

namespace cinder::lua {

class LuaTable {
public:
    static LuaTable fromFile(const std::filesystem::path& path, const char* global);
    ~LuaTable();

    LuaTable(const LuaTable&) = delete;
    LuaTable& operator=(const LuaTable&) = delete;
    LuaTable(LuaTable&& other) noexcept;

    std::string str(const char* key, const std::string& fallback) const;
    int num(const char* key, int fallback) const;

private:
    explicit LuaTable(lua_State* state) : state_(state) {}

    lua_State* state_ = nullptr;
};

}
