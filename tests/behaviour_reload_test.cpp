#include <doctest/doctest.h>

#include "lua/LuaCalls.hpp"
#include "scene/Actor.hpp"
#include "scene/Components.hpp"
#include "scene/Scene.hpp"
#include "script/Behaviour.hpp"
#include "serial/SceneCodec.hpp"

#include <lua.hpp>

#include <cstdint>
#include <memory>
#include <string>

using cinder::scene::Actor;
using cinder::scene::Components;
using cinder::scene::PropRec;
using cinder::scene::Scene;
using cinder::script::Behaviour;
using cinder::serial::SceneCodec;

namespace {

std::string pending;

int readPending(lua_State* state) {
    lua_pushlstring(state, pending.data(), pending.size());
    return 1;
}

const char* HARNESS = R"lua(
function __actor(id) return { id = id } end
task = { spawn = function(fn, self) fn(self) end }
local vec3mt = { __vec = 3 }
function vec3(x, y, z) return setmetatable({ x = x, y = y, z = z }, vec3mt) end
function vecSize(v)
    local mt = type(v) == "table" and getmetatable(v)
    return mt and mt.__vec or nil
end
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

    int ref(const char* expression) {
        const std::string chunk = std::string("return ") + expression;
        REQUIRE(luaL_loadstring(state, chunk.c_str()) == LUA_OK);
        REQUIRE(lua_pcall(state, 0, 1, 0) == LUA_OK);
        return luaL_ref(state, LUA_REGISTRYINDEX);
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

TEST_CASE("pending data is the bag before start") {
    Host host;
    Behaviour* behaviour = host.scene.spawn("subject")->add<Behaviour>(
            host.state, "b.lua", host.ref("{ phase = 3, label = 'box', hook = print }"));

    const PropRec bag = behaviour->readBag();
    CHECK(bag.at("phase").as<std::int64_t>() == 3);
    CHECK(bag.at("label").as<std::string>() == "box");
    CHECK(bag.count("hook") == 0);
}

TEST_CASE("a started behaviour saves its live fields") {
    Host host;
    pending = "return { hits = 0, update = function(self) self.hits = self.hits + 1 end }";

    Behaviour* behaviour = host.attach("b.lua");
    host.scene.update(0.1f);
    host.scene.update(0.1f);

    const PropRec bag = behaviour->readBag();
    CHECK(bag.at("hits").as<std::int64_t>() == 2);
    CHECK(bag.count("update") == 0);
    CHECK(bag.count("actor") == 0);
}

TEST_CASE("fields survive a scene save and load") {
    Host host;
    lua_State* state = host.state;
    host.types.add<Behaviour>("Behaviour", [state] { return std::make_unique<Behaviour>(state); });
    pending = "return { speed = 1, offset = vec3(0, 0, 0), start = function(self) "
              "_G.speed = self.speed; _G.lift = self.offset.y; _G.size = vecSize(self.offset) end }";

    host.scene.spawn(1, "subject", nullptr)
            ->add<Behaviour>(state, "b.lua", host.ref("{ speed = 7, offset = vec3(0, 2.5, 0) }"));

    const std::string text = SceneCodec::save(host.scene);
    CHECK(text.find("speed 7") != std::string::npos);
    CHECK(text.find("offset 0 2.5 0") != std::string::npos);

    SceneCodec::load(text, host.scene);
    CHECK(SceneCodec::save(host.scene) == text);

    host.scene.update(0.1f);
    CHECK(host.probe("speed") == 7);
    CHECK(host.probe("lift") == doctest::Approx(2.5));
    CHECK(host.probe("size") == 3);
}
