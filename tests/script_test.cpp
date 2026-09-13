#include <doctest/doctest.h>

#include "lua/LuaApi.hpp"
#include "lua/LuaCalls.hpp"
#include "lua/LuaSource.hpp"
#include "scene/Actor.hpp"
#include "scene/Components.hpp"
#include "scene/PropValue.hpp"
#include "scene/Scene.hpp"
#include "script/SceneApi.hpp"
#include "script/Script.hpp"

#include <lua.hpp>

#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <utility>

using cinder::scene::Actor;
using cinder::scene::Components;
using cinder::scene::PropValue;
using cinder::scene::Scene;
using cinder::script::Script;

namespace {

const std::filesystem::path PRELUDE = std::filesystem::path(CINDER_SOURCE_DIR) / "engine" / "lua";

std::map<std::string, std::string> files;

int readScript(lua_State* state) {
    const char* file = luaL_checkstring(state, 1);
    const auto found = files.find(file);
    if (found == files.end()) return luaL_error(state, "no script %s", file);
    lua_pushlstring(state, found->second.data(), found->second.size());
    return 1;
}

struct Host {
    lua_State* state = luaL_newstate();
    Components types;
    Scene scene{types};

    Host() {
        files.clear();
        luaL_openlibs(state);
        lua_pushcfunction(state, readScript);
        lua_setglobal(state, "__scriptRead");
        REQUIRE(cinder::lua::runChunk(state, Script::BOOTSTRAP, "=[bootstrap]"));

        lua_State* raw = state;
        types.add<Script>("Script", [raw] { return std::make_unique<Script>(raw); });

        cinder::lua::LuaApi api(state, nullptr);
        cinder::script::registerSceneApi(api, scene);
        api.install("engine");

        for (const char* name : {"types.lua", "scene.lua", "task.lua"}) {
            const std::filesystem::path path = PRELUDE / name;
            const std::string chunk = "@" + path.string();
            REQUIRE(cinder::lua::runChunk(state, cinder::lua::readSource(path), chunk.c_str()));
        }
        cinder::script::listenForAttributes(state, scene);
    }

    ~Host() {
        scene.setAttributeListener(nullptr);
        scene.clear();
        lua_close(state);
    }

    Host(const Host&) = delete;
    Host& operator=(const Host&) = delete;

    Script* attach(const std::string& file, std::string source) {
        files[file] = std::move(source);
        return scene.spawn("subject")->add<Script>(state, file);
    }

    void edit(const std::string& file, std::string source) {
        files[file] = std::move(source);
        lua_getglobal(state, "__scriptForget");
        lua_pushstring(state, file.c_str());
        REQUIRE(lua_pcall(state, 1, 0, 0) == LUA_OK);
    }

    void step() {
        cinder::lua::callGlobal(state, "__step", 0.1);
        scene.update(0.1f);
    }

    double number(const char* name) {
        lua_getglobal(state, name);
        const double value = lua_tonumber(state, -1);
        lua_pop(state, 1);
        return value;
    }

    bool flag(const char* name) {
        lua_getglobal(state, name);
        const bool value = lua_toboolean(state, -1) != 0;
        lua_pop(state, 1);
        return value;
    }

    bool defined(const char* name) {
        lua_getglobal(state, name);
        const bool value = !lua_isnil(state, -1);
        lua_pop(state, 1);
        return value;
    }

    std::string text(const char* name) {
        lua_getglobal(state, name);
        const char* value = lua_tostring(state, -1);
        std::string out = value != nullptr ? value : "";
        lua_pop(state, 1);
        return out;
    }
};

void setEnabled(Script& script, bool enabled) {
    for (const cinder::reflect::PropDef& def : script.propList()) {
        if (def.name() == "enabled") def.writeBool(script.propTarget(), enabled);
    }
}

}

TEST_CASE("a script runs top to bottom once it starts") {
    Host host;
    Script* script = host.attach("a.lua", "_G.runs = (_G.runs or 0) + 1\n"
                                          "_G.actorId = script.actor.id\n"
                                          "_G.file = script.file\n");
    CHECK_FALSE(host.defined("runs"));

    host.step();
    CHECK(host.number("runs") == 1);
    CHECK(host.number("actorId") == script->actor()->id());
    CHECK(host.text("file") == "a.lua");

    host.step();
    CHECK(host.number("runs") == 1);
}

TEST_CASE("a script's globals stay in its own environment") {
    Host host;
    host.attach("a.lua", "hidden = 1\n_G.shared = 2\n");

    host.step();
    CHECK_FALSE(host.defined("hidden"));
    CHECK(host.number("shared") == 2);
}

TEST_CASE("destroying an actor stops what its script started") {
    Host host;
    Script* script = host.attach(
            "a.lua",
            "stepped:connect(function() _G.ticks = (_G.ticks or 0) + 1 end)\n"
            "task.spawn(function() while true do task.wait(0) _G.loops = (_G.loops or 0) + 1 end end)\n");
    host.step();
    host.step();
    CHECK(host.number("ticks") > 0);
    CHECK(host.number("loops") > 0);

    script->actor()->destroy();
    host.step();
    const double ticks = host.number("ticks");
    const double loops = host.number("loops");

    host.step();
    host.step();
    CHECK(host.number("ticks") == ticks);
    CHECK(host.number("loops") == loops);
}

TEST_CASE("disabling a script stops it and enabling runs it again") {
    Host host;
    Script* script = host.attach("a.lua",
                                 "_G.runs = (_G.runs or 0) + 1\n"
                                 "stepped:connect(function() _G.ticks = (_G.ticks or 0) + 1 end)\n");
    host.step();
    host.step();
    CHECK(host.number("runs") == 1);

    setEnabled(*script, false);
    const double ticks = host.number("ticks");
    host.step();
    CHECK(host.number("ticks") == ticks);

    setEnabled(*script, true);
    CHECK(host.number("runs") == 2);
    host.step();
    CHECK(host.number("ticks") == ticks + 1);
}

TEST_CASE("a reload runs the new file, stops the old one and keeps attributes") {
    Host host;
    Script* script = host.attach("a.lua", "stepped:connect(function() _G.old = (_G.old or 0) + 1 end)\n");
    script->actor()->setAttribute("speed", PropValue::integer(5));
    host.step();
    host.step();

    host.edit("a.lua", "stepped:connect(function() _G.speed = script.actor:getAttribute('speed') end)\n");
    script->reload();
    const double old = host.number("old");
    host.step();

    CHECK(host.number("old") == old);
    CHECK(host.number("speed") == 5);
}

TEST_CASE("a broken edit leaves the running script alone") {
    Host host;
    Script* script = host.attach("a.lua", "stepped:connect(function() _G.ticks = (_G.ticks or 0) + 1 end)\n");
    host.step();
    host.step();

    host.edit("a.lua", "stepped:connect(function()\n");
    script->reload();
    const double ticks = host.number("ticks");
    host.step();

    CHECK(host.number("ticks") == ticks + 1);
}

TEST_CASE("a broken file fails the check with its own location") {
    Host host;
    host.edit("a.lua", "stepped:connect(function()\n");

    lua_getglobal(host.state, "__scriptCheck");
    lua_pushstring(host.state, "a.lua");
    REQUIRE(lua_pcall(host.state, 1, 0, 0) != LUA_OK);
    const std::string message = lua_tostring(host.state, -1);
    lua_pop(host.state, 1);

    CHECK(message.rfind("a.lua:", 0) == 0);
}

TEST_CASE("a script that never started does not run on reload") {
    Host host;
    Script* script = host.attach("a.lua", "_G.runs = 1\n");

    script->reload();
    CHECK_FALSE(host.defined("runs"));
}

TEST_CASE("attribute changes from C++ reach both changed signals") {
    Host host;
    Script* script = host.attach(
            "a.lua",
            "local actor = script.actor\n"
            "actor:getAttributeChangedSignal('hp'):connect(function() _G.hp = (_G.hp or 0) + 1 end)\n"
            "actor.attributeChanged:connect(function(name) _G.changed = name end)\n");
    host.step();

    Actor& actor = *script->actor();
    actor.setAttribute("hp", PropValue::integer(3));
    actor.setAttribute("hp", PropValue::number(3.0));
    CHECK(host.number("hp") == 1);
    CHECK(host.text("changed") == "hp");

    actor.removeAttribute("hp");
    CHECK(host.number("hp") == 2);
}

TEST_CASE("attributes hold vectors and reject tables") {
    Host host;
    host.attach("a.lua",
                "_G.ok = pcall(function() script.actor:setAttribute('bad', {}) end)\n"
                "script.actor:setAttribute('offset', vec3(1, 2, 3))\n"
                "_G.y = script.actor:getAttribute('offset').y\n");
    host.step();

    CHECK(host.defined("ok"));
    CHECK_FALSE(host.flag("ok"));
    CHECK(host.number("y") == 2);
}
