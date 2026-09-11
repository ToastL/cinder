#include "script/Behaviour.hpp"

#include "lua/LuaCalls.hpp"
#include "platform/Log.hpp"
#include "scene/Actor.hpp"

#include <utility>

namespace cinder::script {

const char* Behaviour::BOOTSTRAP = R"lua(
local protos = {}
function __behaviourNew(path, actor, data)
    local proto = protos[path]
    if proto == nil then
        proto = assert(load(__behaviourRead(path), path))()
        protos[path] = proto
    end
    local t = { actor = __actor(actor) }
    for k, v in pairs(proto) do t[k] = v end
    if data then for k, v in pairs(data) do t[k] = v end end
    if t.start then
        local inner = t.start
        t.start = function(self) task.spawn(inner, self) end
    end
    return t
end

function __behaviourForget(path)
    protos[path] = nil
end

function __behaviourFields(instance)
    local fields = {}
    for k, v in pairs(instance) do
        if k ~= "actor" and type(v) ~= "function" then fields[k] = v end
    end
    return fields
end
)lua";

Behaviour::Behaviour(lua_State* state) : state_(state) {}

Behaviour::Behaviour(lua_State* state, std::string script, int data)
    : state_(state), script_(std::move(script)), data_(data) {}

int Behaviour::instantiate(int data) {
    if (script_.empty()) return LUA_NOREF;

    const int top = lua_gettop(state_);

    lua_getglobal(state_, "__behaviourNew");
    lua_pushstring(state_, script_.c_str());
    lua_pushinteger(state_, actor()->id());
    if (data == LUA_NOREF) lua_pushnil(state_);
    else lua_rawgeti(state_, LUA_REGISTRYINDEX, data);

    if (lua_pcall(state_, 3, 1, 0) != LUA_OK) {
        const char* message = lua_tostring(state_, -1);
        cinder::platform::logError("[lua] %s: %s\n", script_.c_str(),
                                   message != nullptr ? message : "unknown error");
        lua_settop(state_, top);
        return LUA_NOREF;
    }

    const int ref = luaL_ref(state_, LUA_REGISTRYINDEX);
    lua_settop(state_, top);
    return ref;
}

int Behaviour::snapshot() {
    const int top = lua_gettop(state_);

    lua_getglobal(state_, "__behaviourFields");
    lua_rawgeti(state_, LUA_REGISTRYINDEX, ref_);

    if (lua_pcall(state_, 1, 1, 0) != LUA_OK) {
        const char* message = lua_tostring(state_, -1);
        cinder::platform::logError("[lua] %s: %s\n", script_.c_str(),
                                   message != nullptr ? message : "unknown error");
        lua_settop(state_, top);
        return LUA_NOREF;
    }

    const int fields = luaL_ref(state_, LUA_REGISTRYINDEX);
    lua_settop(state_, top);
    return fields;
}

void Behaviour::reload() {
    if (ref_ == LUA_NOREF) return;

    const int fields = snapshot();
    const int next = instantiate(fields);
    luaL_unref(state_, LUA_REGISTRYINDEX, fields);
    if (next == LUA_NOREF) return;

    luaL_unref(state_, LUA_REGISTRYINDEX, ref_);
    ref_ = next;
}

void Behaviour::release() {
    if (data_ == LUA_NOREF) return;
    luaL_unref(state_, LUA_REGISTRYINDEX, data_);
    data_ = LUA_NOREF;
}

void Behaviour::onStart() {
    ref_ = instantiate(data_);
    release();
    if (ref_ != LUA_NOREF) cinder::lua::callMethod(state_, ref_, "start");
}

void Behaviour::onUpdate(float dt) {
    if (ref_ != LUA_NOREF) cinder::lua::callMethod(state_, ref_, "update", static_cast<double>(dt));
}

void Behaviour::onDestroy() {
    if (ref_ != LUA_NOREF) {
        cinder::lua::callMethod(state_, ref_, "destroy");
        luaL_unref(state_, LUA_REGISTRYINDEX, ref_);
        ref_ = LUA_NOREF;
    }
    release();
}

Behaviour::~Behaviour() { release(); }

}
