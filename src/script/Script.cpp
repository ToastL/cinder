#include "script/Script.hpp"

#include "lua/LuaCalls.hpp"

#include <utility>

namespace cinder::script {

const char* Script::BOOTSTRAP = R"lua(
local sources = {}

local function sourceOf(file)
    local source = sources[file]
    if source == nil then
        source = __scriptRead(file)
        sources[file] = source
    end
    return source
end

local function compile(file, env)
    local chunk, err = load(sourceOf(file), "@" .. file, "t", env)
    if chunk == nil then error(err, 0) end
    return chunk
end

function __scriptCheck(file)
    compile(file, {})
end

function __scriptStart(file, node, previous)
    local env = setmetatable({ script = __node(node) }, { __index = _G })
    local chunk = compile(file, env)

    if previous then __taskStop(previous) end
    local owner = __taskOwner()
    __taskSpawnAs(owner, chunk)
    return owner
end

function __scriptStop(owner)
    __taskStop(owner)
end

function __scriptForget(file)
    sources[file] = nil
end
)lua";

Script::Script(lua_State* state) : state_(state) {}

Script::Script(lua_State* state, std::string file) : state_(state), file_(std::move(file)) {}

void Script::start() {
    if (file_.empty() || scene() == nullptr) return;

    cinder::lua::StackRestore stack(state_);

    lua_getglobal(state_, "__scriptStart");
    lua_pushstring(state_, file_.c_str());
    lua_pushinteger(state_, id());
    if (owner_ == LUA_NOREF) lua_pushnil(state_);
    else lua_rawgeti(state_, LUA_REGISTRYINDEX, owner_);

    if (!cinder::lua::protectedCall(state_, 3, 1)) return;

    if (owner_ != LUA_NOREF) luaL_unref(state_, LUA_REGISTRYINDEX, owner_);
    owner_ = luaL_ref(state_, LUA_REGISTRYINDEX);
}

void Script::stop() {
    if (owner_ == LUA_NOREF) return;

    cinder::lua::StackRestore stack(state_);

    lua_getglobal(state_, "__scriptStop");
    lua_rawgeti(state_, LUA_REGISTRYINDEX, owner_);
    cinder::lua::protectedCall(state_, 1, 0, file_.c_str());

    luaL_unref(state_, LUA_REGISTRYINDEX, owner_);
    owner_ = LUA_NOREF;
}

void Script::onStart() {
    if (isEnabled()) start();
}

void Script::onDestroy() { stop(); }

void Script::propChanged(const cinder::reflect::PropDef& prop) {
    if (prop.name() != "enabled" || !started()) return;
    if (!isEnabled()) stop();
    else if (!running()) start();
}

void Script::reload() {
    if (started() && isEnabled()) start();
}

Script::~Script() {
    if (owner_ != LUA_NOREF) luaL_unref(state_, LUA_REGISTRYINDEX, owner_);
}

}
