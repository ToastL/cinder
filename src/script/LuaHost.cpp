#include "script/LuaHost.hpp"

#include "gfx/Renderer.hpp"
#include "lua/LuaApi.hpp"
#include "lua/LuaCalls.hpp"
#include "lua/LuaSource.hpp"
#include "platform/Assets.hpp"
#include "platform/Glfw.hpp"
#include "platform/Input.hpp"
#include "scene/Components.hpp"
#include "scene/Scene.hpp"
#include "script/Behaviour.hpp"
#include "script/SceneApi.hpp"

#include <cstdio>
#include <string>
#include <utility>

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

int behaviourRead(lua_State* state) {
    const char* path = lua_tostring(state, 1);
    if (path == nullptr) return luaL_error(state, "__behaviourRead expects a path");

    try {
        const std::string source = cinder::lua::readSource(cinder::platform::resolveAsset(path));
        lua_pushlstring(state, source.data(), source.size());
        return 1;
    } catch (const std::exception& e) {
        return luaL_error(state, "%s", e.what());
    }
}

int setClearColor(lua_State* state) {
    host(state).renderer->setClearColor(static_cast<float>(lua_tonumber(state, 1)),
                                        static_cast<float>(lua_tonumber(state, 2)),
                                        static_cast<float>(lua_tonumber(state, 3)));
    return 0;
}

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
    std::printf("[game] %s\n", message != nullptr ? message : "nil");
    lua_pop(state, 1);
    return 0;
}

int loadTexture(lua_State* state) {
    lua_pushinteger(state, host(state).renderer->assets().load(lua_tostring(state, 1)));
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

LuaHost::LuaHost(std::filesystem::path source, cinder::scene::Scene& scene,
                 cinder::platform::Input& input, cinder::gfx::Renderer& renderer,
                 std::function<void()> quit)
    : source_(std::move(source)), scene_(scene), input_(input), renderer_(renderer),
      quit_(std::move(quit)) {}

void LuaHost::load() {
    scene_.clear();
    close();

    state_ = luaL_newstate();
    luaL_openlibs(state_);

    registerScripts();
    registerApi();
    loadPrelude();

    const std::string source = cinder::lua::readSource(source_);
    lastModified_ = cinder::lua::modifiedMillis(source_, lastModified_);

    if (luaL_dostring(state_, source.c_str()) != LUA_OK) {
        std::fprintf(stderr, "[lua] load error: %s\n", lua_tostring(state_, -1));
        lua_pop(state_, 1);
        return;
    }
    std::printf("[lua] loaded %s\n", source_.string().c_str());
}

void LuaHost::registerScripts() {
    lua_pushcfunction(state_, behaviourRead);
    lua_setglobal(state_, "__behaviourRead");

    if (luaL_dostring(state_, Behaviour::BOOTSTRAP) != LUA_OK) {
        std::fprintf(stderr, "[lua] bootstrap: %s\n", lua_tostring(state_, -1));
        lua_pop(state_, 1);
    }

    lua_State* state = state_;
    scene_.types().add<Behaviour>("Behaviour",
                                  [state] { return std::make_unique<Behaviour>(state); });
}

void LuaHost::registerApi() {
    hostContext = Host{&renderer_, &input_, &quit_};

    LuaApi api(state_, &hostContext);

    api.bind("setClearColor", setClearColor);
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
            if (luaL_dostring(state_, source.c_str()) != LUA_OK) {
                std::fprintf(stderr, "[lua] prelude %s: %s\n", relative,
                             lua_tostring(state_, -1));
                lua_pop(state_, 1);
            }
        } catch (const std::exception& e) {
            std::fprintf(stderr, "[lua] prelude %s: %s\n", relative, e.what());
        }
    }
}

void LuaHost::poll() {
    if (cinder::lua::modifiedMillis(source_, lastModified_) != lastModified_) load();
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
