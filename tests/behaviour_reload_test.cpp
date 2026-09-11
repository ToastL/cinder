#include <doctest/doctest.h>

#include "lua/LuaCalls.hpp"
#include "scene/Actor.hpp"
#include "scene/Components.hpp"
#include "scene/Scene.hpp"
#include "script/Behaviour.hpp"

#include <lua.hpp>

#include <memory>
#include <string>

using cinder::scene::Actor;
using cinder::scene::Components;
using cinder::scene::Scene;
using cinder::script::Behaviour;

namespace {

std::string pending;

int readPending(lua_State* state) {
    lua_pushlstring(state, pending.data(), pending.size());
    return 1;
}

const char* HARNESS = R"lua(
function __actor(id) return { id = id } end
task = { spawn = function(fn, self) fn(self) end }
)lua";

struct Host {
    lua_State* state = luaL_newstate();
    Components types;
    Scene scene{types};

    Host() {
        luaL_openlibs(state);
        lua_pushcfunction(state, readPending);
        lua_setglobal(state, "__behaviourRead");
        REQUIRE(cinder::lua::runChunk(state, HARNESS, "=[harness]"));
        REQUIRE(cinder::lua::runChunk(state, Behaviour::BOOTSTRAP, "=[bootstrap]"));
    }

    ~Host() {
        scene.clear();
        lua_close(state);
    }

    Host(const Host&) = delete;
    Host& operator=(const Host&) = delete;

    Behaviour* attach(std::string script) {
        Actor* actor = scene.spawn("subject");
        auto owned = std::make_unique<Behaviour>(state, std::move(script), LUA_NOREF);
        Behaviour* raw = owned.get();
        actor->add(std::move(owned));
        return raw;
    }

    void forget(const char* path) {
        lua_getglobal(state, "__behaviourForget");
        lua_pushstring(state, path);
        REQUIRE(lua_pcall(state, 1, 0, 0) == LUA_OK);
    }

    double probe(const char* name) {
        lua_getglobal(state, name);
        const double value = lua_tonumber(state, -1);
        lua_pop(state, 1);
        return value;
    }
};

}

TEST_CASE("a reload swaps in the new code") {
    Host host;
    pending = "return { update = function(self) _G.mark = 1 end }";

    Behaviour* behaviour = host.attach("b.lua");
    host.scene.update(0.1f);
    CHECK(host.probe("mark") == 1);

    pending = "return { update = function(self) _G.mark = 2 end }";
    host.forget("b.lua");
    behaviour->reload();

    host.scene.update(0.1f);
    CHECK(host.probe("mark") == 2);
}

TEST_CASE("fields survive a reload") {
    Host host;
    pending = "return { hits = 0, update = function(self) self.hits = self.hits + 1 end }";

    Behaviour* behaviour = host.attach("b.lua");
    for (int i = 0; i < 3; ++i) host.scene.update(0.1f);

    pending = "return { hits = 0, update = function(self) _G.hits = self.hits end }";
    host.forget("b.lua");
    behaviour->reload();

    host.scene.update(0.1f);
    CHECK(host.probe("hits") == 3);
}

TEST_CASE("a broken edit leaves the running instance alone") {
    Host host;
    pending = "return { update = function(self) _G.mark = 1 end }";

    Behaviour* behaviour = host.attach("b.lua");
    host.scene.update(0.1f);
    CHECK(host.probe("mark") == 1);

    pending = "return { update = function(self) _G.mark = 2 end";
    host.forget("b.lua");
    behaviour->reload();

    lua_pushnil(host.state);
    lua_setglobal(host.state, "mark");
    host.scene.update(0.1f);
    CHECK(host.probe("mark") == 1);
}

TEST_CASE("an instance that never started is not reloadable") {
    Host host;
    pending = "return { update = function(self) _G.mark = 1 end }";

    Behaviour behaviour(host.state, "b.lua", LUA_NOREF);
    behaviour.reload();
    CHECK(host.probe("mark") == 0);
}
