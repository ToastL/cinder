#include "script/LuaProps.hpp"

#include <string>
#include <utility>

namespace cinder::script {
namespace {

using cinder::scene::PropRec;
using cinder::scene::PropSeq;
using cinder::scene::PropValue;

constexpr int MAX_DEPTH = 16;
const char* AXES[] = {"x", "y", "z", "w"};

std::optional<PropValue> read(lua_State* state, int index, int depth);

PropRec record(lua_State* state, int index, int depth) {
    PropRec out;
    lua_pushnil(state);
    while (lua_next(state, index) != 0) {
        if (lua_type(state, -2) == LUA_TSTRING) {
            if (std::optional<PropValue> value = read(state, lua_gettop(state), depth)) {
                out.insert_or_assign(lua_tostring(state, -2), std::move(*value));
            }
        }
        lua_pop(state, 1);
    }
    return out;
}

int vectorArity(lua_State* state, int index) {
    if (!lua_getmetatable(state, index)) return 0;
    lua_getfield(state, -1, "__vec");
    const int arity = lua_isinteger(state, -1) ? static_cast<int>(lua_tointeger(state, -1)) : -1;
    lua_pop(state, 2);
    return arity;
}

PropValue vector(lua_State* state, int index, int arity) {
    PropSeq values;
    for (int i = 0; i < arity && i < 4; ++i) {
        lua_getfield(state, index, AXES[i]);
        values.push_back(PropValue::number(lua_tonumber(state, -1)));
        lua_pop(state, 1);
    }
    return PropValue::seq(std::move(values));
}

std::optional<PropValue> sequence(lua_State* state, int index, lua_Unsigned length) {
    PropSeq values;
    for (lua_Unsigned i = 1; i <= length; ++i) {
        lua_rawgeti(state, index, static_cast<lua_Integer>(i));
        std::optional<PropValue> value;
        if (lua_type(state, -1) != LUA_TTABLE) value = read(state, lua_gettop(state), 0);
        lua_pop(state, 1);
        if (!value) return std::nullopt;
        values.push_back(std::move(*value));
    }
    return PropValue::seq(std::move(values));
}

std::optional<PropValue> table(lua_State* state, int index, int depth) {
    if (depth >= MAX_DEPTH || !lua_checkstack(state, 4)) return std::nullopt;

    const int arity = vectorArity(state, index);
    if (arity > 0) return vector(state, index, arity);
    if (arity < 0) return std::nullopt;

    const lua_Unsigned length = lua_rawlen(state, index);
    if (length > 0) return sequence(state, index, length);
    return PropValue::rec(record(state, index, depth + 1));
}

std::optional<PropValue> read(lua_State* state, int index, int depth) {
    switch (lua_type(state, index)) {
        case LUA_TBOOLEAN:
            return PropValue::flag(lua_toboolean(state, index) != 0);
        case LUA_TNUMBER:
            if (lua_isinteger(state, index)) return PropValue::integer(lua_tointeger(state, index));
            return PropValue::number(lua_tonumber(state, index));
        case LUA_TSTRING: {
            std::size_t length = 0;
            const char* text = lua_tolstring(state, index, &length);
            return PropValue::text(std::string(text, length));
        }
        case LUA_TTABLE:
            return table(state, index, depth);
        default:
            return std::nullopt;
    }
}

}

void pushProp(lua_State* state, const PropValue& value) {
    if (!lua_checkstack(state, 3)) return;

    if (value.is<std::int64_t>()) {
        lua_pushinteger(state, static_cast<lua_Integer>(value.as<std::int64_t>()));
    } else if (value.is<double>()) {
        lua_pushnumber(state, value.as<double>());
    } else if (value.is<std::string>()) {
        const std::string& text = value.as<std::string>();
        lua_pushlstring(state, text.data(), text.size());
    } else if (value.is<bool>()) {
        lua_pushboolean(state, value.as<bool>());
    } else if (value.is<PropSeq>()) {
        const PropSeq& values = value.as<PropSeq>();
        lua_createtable(state, static_cast<int>(values.size()), 0);
        for (std::size_t i = 0; i < values.size(); ++i) {
            pushProp(state, values[i]);
            lua_rawseti(state, -2, static_cast<lua_Integer>(i) + 1);
        }
    } else {
        pushRec(state, value.as<PropRec>());
    }
}

void pushRec(lua_State* state, const PropRec& values) {
    lua_createtable(state, 0, static_cast<int>(values.size()));
    for (const auto& [key, value] : values) {
        pushProp(state, value);
        lua_setfield(state, -2, key.c_str());
    }
}

std::optional<PropValue> readProp(lua_State* state, int index) {
    return read(state, lua_absindex(state, index), 0);
}

PropRec readRec(lua_State* state, int index) {
    index = lua_absindex(state, index);
    if (lua_type(state, index) != LUA_TTABLE) return {};
    return record(state, index, 0);
}

}
