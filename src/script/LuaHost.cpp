#include "script/LuaHost.hpp"

#include "gfx/Renderer.hpp"
#include "lua/LuaApi.hpp"
#include "lua/LuaCalls.hpp"
#include "lua/LuaSource.hpp"
#include "platform/Assets.hpp"
#include "platform/Glfw.hpp"
#include "platform/Input.hpp"
#include "platform/Log.hpp"
#include "scene/Actor.hpp"
#include "scene/Components.hpp"
#include "scene/Scene.hpp"
#include "script/Behaviour.hpp"
#include "script/SceneApi.hpp"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace cinder::script {
namespace {

using cinder::lua::LuaApi;

const char* PRELUDE[] = {
    "scripts/lib/types.lua",
    "scripts/lib/scene.lua",
    "scripts/lib/task.lua",
};

struct Host {
    cinder::gfx::Renderer* renderer;
    cinder::platform::Input* input;
    std::function<void()>* quit;
};

Host& host(lua_State* state) { return *LuaApi::context<Host>(state); }

int time(lua_State* state) {
    lua_pushnumber(state, cinder::platform::Glfw::time());
    return 1;
}

int quit(lua_State* state) {
    (*host(state).quit)();
    return 0;
}

int logMessage(lua_State* state) {
    const char* message = luaL_tolstring(state, 1, nullptr);
    cinder::platform::logInfo("[game] %s\n", message != nullptr ? message : "nil");
    lua_pop(state, 1);
    return 0;
}

int loadTexture(lua_State* state) {
    const std::string path = cinder::platform::resolveAsset(luaL_checkstring(state, 1)).string();
    lua_pushinteger(state, host(state).renderer->assets().load(path));
    return 1;
}

int textureSize(lua_State* state) {
    const auto& texture = host(state).renderer->assets().get(
            static_cast<int>(lua_tointeger(state, 1)));
    lua_pushinteger(state, texture.width());
    lua_pushinteger(state, texture.height());
    return 2;
}

int keyDown(lua_State* state) {
    lua_pushboolean(state, host(state).input->keyDown(lua_tostring(state, 1)));
    return 1;
}

int keyPressed(lua_State* state) {
    lua_pushboolean(state, host(state).input->keyPressed(lua_tostring(state, 1)));
    return 1;
}

int keyReleased(lua_State* state) {
    lua_pushboolean(state, host(state).input->keyReleased(lua_tostring(state, 1)));
    return 1;
}

int mouseDown(lua_State* state) {
    lua_pushboolean(state, host(state).input->mouseDown(lua_tostring(state, 1)));
    return 1;
}

int mousePressed(lua_State* state) {
    lua_pushboolean(state, host(state).input->mousePressed(lua_tostring(state, 1)));
    return 1;
}

int mouseReleased(lua_State* state) {
    lua_pushboolean(state, host(state).input->mouseReleased(lua_tostring(state, 1)));
    return 1;
}

int mousePosition(lua_State* state) {
    lua_pushnumber(state, host(state).input->mouseX());
    lua_pushnumber(state, host(state).input->mouseY());
    return 2;
}

int mouseDelta(lua_State* state) {
    lua_pushnumber(state, host(state).input->mouseDeltaX());
    lua_pushnumber(state, host(state).input->mouseDeltaY());
    return 2;
}

int scroll(lua_State* state) {
    lua_pushnumber(state, host(state).input->scrollX());
    lua_pushnumber(state, host(state).input->scrollY());
    return 2;
}

int setCursorLocked(lua_State* state) {
    host(state).input->setCursorLocked(lua_toboolean(state, 1) != 0);
    return 0;
}

int cursorLocked(lua_State* state) {
    lua_pushboolean(state, host(state).input->cursorLocked());
    return 1;
}

Host hostContext;

}

LuaHost::LuaHost(cinder::scene::Scene& scene, cinder::platform::Input& input,
                 cinder::gfx::Renderer& renderer, std::function<void()> quit)
    : scene_(scene), input_(input), renderer_(renderer), quit_(std::move(quit)) {}

int LuaHost::behaviourRead(lua_State* state) {
    const char* path = lua_tostring(state, 1);
    if (path == nullptr) return luaL_error(state, "__behaviourRead expects a path");

    try {
        const std::string source = cinder::lua::readSource(cinder::platform::resolveAsset(path));
        LuaApi::context<LuaHost>(state)->watch(path);
        lua_pushlstring(state, source.data(), source.size());
        return 1;
    } catch (const std::exception& e) {
        return luaL_error(state, "%s", e.what());
    }
}

void LuaHost::boot() {
    close();
    watched_.clear();

    state_ = luaL_newstate();
    luaL_openlibs(state_);

    registerScripts();
    registerApi();
    loadPrelude();
}

void LuaHost::registerScripts() {
    lua_pushlightuserdata(state_, this);
    lua_pushcclosure(state_, behaviourRead, 1);
    lua_setglobal(state_, "__behaviourRead");

    if (!cinder::lua::runChunk(state_, Behaviour::BOOTSTRAP, "=[behaviour bootstrap]")) {
        cinder::platform::logError("[lua] bootstrap: %s\n", lua_tostring(state_, -1));
        lua_pop(state_, 1);
    }

    lua_State* state = state_;
    scene_.types().add<Behaviour>("Behaviour",
                                  [state] { return std::make_unique<Behaviour>(state); });
}

void LuaHost::registerApi() {
    hostContext = Host{&renderer_, &input_, &quit_};

    LuaApi api(state_, &hostContext);

    api.bind("time", time);
    api.bind("quit", quit);
    api.bind("log", logMessage);
    api.bind("loadTexture", loadTexture);
    api.bind("textureSize", textureSize);

    api.bind("keyDown", keyDown);
    api.bind("keyPressed", keyPressed);
    api.bind("keyReleased", keyReleased);

    api.bind("mouseDown", mouseDown);
    api.bind("mousePressed", mousePressed);
    api.bind("mouseReleased", mouseReleased);

    api.bind("mousePosition", mousePosition);
    api.bind("mouseDelta", mouseDelta);
    api.bind("scroll", scroll);
    api.bind("setCursorLocked", setCursorLocked);
    api.bind("cursorLocked", cursorLocked);

    registerSceneApi(api, scene_);
    renderer_.registerApi(api);

    api.install("engine");
}

void LuaHost::loadPrelude() {
    for (const char* relative : PRELUDE) {
        const std::filesystem::path path = cinder::platform::assetPath(relative);
        try {
            const std::string source = cinder::lua::readSource(path);
            const std::string chunk = "@" + path.string();
            if (!cinder::lua::runChunk(state_, source, chunk.c_str())) {
                cinder::platform::logError("[lua] prelude %s: %s\n", relative,
                                           lua_tostring(state_, -1));
                lua_pop(state_, 1);
            }
        } catch (const std::exception& e) {
            cinder::platform::logError("[lua] prelude %s: %s\n", relative, e.what());
        }
    }
}

void LuaHost::watch(const char* path) {
    const std::filesystem::path file = cinder::platform::resolveAsset(path);
    watched_.insert({path, Watch{file, cinder::lua::modifiedMillis(file, 0)}});
}

void LuaHost::poll() {
    std::vector<std::string> changed;
    for (auto& [path, entry] : watched_) {
        const std::int64_t modified = cinder::lua::modifiedMillis(entry.file, entry.modified);
        if (modified == entry.modified) continue;
        entry.modified = modified;
        changed.push_back(path);
    }

    for (const std::string& path : changed) reloadBehaviour(path);
}

void LuaHost::reloadBehaviour(const std::string& path) {
    const int top = lua_gettop(state_);
    lua_getglobal(state_, "__behaviourForget");
    lua_pushstring(state_, path.c_str());
    if (lua_pcall(state_, 1, 0, 0) != LUA_OK) {
        cinder::platform::logError("[lua] reload %s: %s\n", path.c_str(), lua_tostring(state_, -1));
        lua_settop(state_, top);
        return;
    }
    lua_settop(state_, top);

    int count = 0;
    for (cinder::scene::Actor* root : scene_.roots()) count += reloadIn(*root, path);
    cinder::platform::logInfo("[lua] reloaded %s (%d)\n", path.c_str(), count);
}

int LuaHost::reloadIn(cinder::scene::Actor& actor, const std::string& path) {
    int count = 0;

    for (const std::unique_ptr<cinder::scene::Component>& component : actor.components()) {
        auto* behaviour = dynamic_cast<Behaviour*>(component.get());
        if (behaviour == nullptr || behaviour->script() != path) continue;
        behaviour->reload();
        ++count;
    }

    for (cinder::scene::Actor* child : actor.children()) count += reloadIn(*child, path);
    return count;
}

void LuaHost::eval(const std::string& source) {
    if (state_ == nullptr) return;

    const std::string expression = "return " + source;
    if (luaL_loadbuffer(state_, expression.data(), expression.size(), "=[console]") != LUA_OK) {
        lua_pop(state_, 1);
        if (luaL_loadbuffer(state_, source.data(), source.size(), "=[console]") != LUA_OK) {
            cinder::platform::logError("[lua] %s\n", lua_tostring(state_, -1));
            lua_pop(state_, 1);
            return;
        }
    }

    const int top = lua_gettop(state_) - 1;
    if (lua_pcall(state_, 0, LUA_MULTRET, 0) != LUA_OK) {
        cinder::platform::logError("[lua] %s\n", lua_tostring(state_, -1));
        lua_settop(state_, top);
        return;
    }

    for (int i = top + 1; i <= lua_gettop(state_); ++i) {
        cinder::platform::logInfo("[lua] %s\n", luaL_tolstring(state_, i, nullptr));
        lua_pop(state_, 1);
    }
    lua_settop(state_, top);
}

void LuaHost::update(float dt) {
    cinder::lua::callGlobal(state_, "__step", static_cast<double>(dt));
}

void LuaHost::render(float alpha) {
    cinder::lua::callGlobal(state_, "__render", static_cast<double>(alpha));
}

void LuaHost::close() {
    if (state_ == nullptr) return;
    lua_close(state_);
    state_ = nullptr;
}

LuaHost::~LuaHost() { close(); }

}
