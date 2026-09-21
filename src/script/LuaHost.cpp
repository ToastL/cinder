#include "script/LuaHost.hpp"

#include "gfx/Renderer.hpp"
#include "lua/LuaApi.hpp"
#include "lua/LuaCalls.hpp"
#include "lua/LuaSource.hpp"
#include "platform/Assets.hpp"
#include "platform/Log.hpp"
#include "scene/Node.hpp"
#include "scene/NodeTypes.hpp"
#include "scene/Scene.hpp"
#include "physics/World.hpp"
#include "script/SceneApi.hpp"
#include "script/SceneObservers.hpp"
#include "script/Script.hpp"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace cinder::script {
namespace {

using cinder::lua::LuaApi;

const char* PRELUDE[] = {
    "lua/types.lua",
    "lua/scene.lua",
    "lua/task.lua",
};

bool callWithPath(lua_State* state, const char* global, const std::string& path) {
    cinder::lua::StackRestore stack(state);
    lua_getglobal(state, global);
    lua_pushstring(state, path.c_str());
    return cinder::lua::protectedCall(state, 1, 0);
}

}

LuaHost::LuaHost(cinder::scene::Scene& scene, cinder::platform::Input& input,
                 cinder::gfx::Renderer& renderer, cinder::physics::World& physics,
                 std::function<void()> quit)
    : scene_(scene), input_(input), renderer_(renderer), physics_(physics), quit_(std::move(quit)),
      runtime_{&renderer_, &input_, &quit_} {}

int LuaHost::scriptRead(lua_State* state) {
    const char* path = lua_tostring(state, 1);
    if (path == nullptr) return luaL_error(state, "__scriptRead expects a path");

    try {
        const std::filesystem::path file = cinder::platform::sourcePath(path);
        const std::string source = cinder::lua::readSource(file);
        LuaApi::context<LuaHost>(state)->watch(path, file);
        lua_pushlstring(state, source.data(), source.size());
        return 1;
    } catch (const std::exception& e) {
        luaL_where(state, 1);
        lua_pushstring(state, e.what());
        lua_concat(state, 2);
    }
    return lua_error(state);
}

void LuaHost::boot() {
    close();
    watched_.clear();

    state_ = luaL_newstate();
    luaL_openlibs(state_);

    registerScripts();
    registerApi();
    loadPrelude();
    observer_ = makeSceneObserver(state_);
    scene_.setObserver(observer_.get());
    contacts_ = makeContactObserver(state_);
    physics_.setObserver(contacts_.get());
}

void LuaHost::registerScripts() {
    lua_pushlightuserdata(state_, this);
    lua_pushcclosure(state_, scriptRead, 1);
    lua_setglobal(state_, "__scriptRead");

    if (!cinder::lua::runChunk(state_, Script::BOOTSTRAP, "=[script bootstrap]")) {
        cinder::platform::logError("[lua] bootstrap: %s\n", lua_tostring(state_, -1));
        lua_pop(state_, 1);
    }

    lua_State* state = state_;
    scene_.types().add<Script>("Script", [state] { return std::make_unique<Script>(state); });
}

void LuaHost::registerApi() {
    LuaApi api(state_, nullptr);
    registerRuntimeApi(api, runtime_);

    registerSceneApi(api, scene_);
    renderer_.registerApi(api);
    physics_.registerApi(api);

    api.install("engine");
}

void LuaHost::loadPrelude() {
    for (const char* relative : PRELUDE) {
        const std::filesystem::path path = cinder::platform::enginePath(relative);
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

void LuaHost::watch(const char* path, const std::filesystem::path& file) {
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

    for (const std::string& path : changed) reloadScript(path);
}

void LuaHost::reloadScript(const std::string& path) {
    if (!callWithPath(state_, "__scriptForget", path)) return;
    if (!callWithPath(state_, "__scriptCheck", path)) return;

    int count = 0;
    const std::vector<cinder::scene::Node*> roots = scene_.roots();
    for (cinder::scene::Node* root : roots) count += reloadIn(*root, path);
    cinder::platform::logInfo("[lua] reloaded %s (%d)\n", path.c_str(), count);
}

int LuaHost::reloadIn(cinder::scene::Node& node, const std::string& path) {
    int count = 0;

    auto* script = dynamic_cast<Script*>(&node);
    if (script != nullptr && script->file() == path) {
        script->reload();
        ++count;
    }

    const std::vector<cinder::scene::Node*> children = node.children();
    for (cinder::scene::Node* child : children) count += reloadIn(*child, path);
    return count;
}

void LuaHost::eval(const std::string& source) {
    if (state_ == nullptr) return;

    cinder::lua::StackRestore stack(state_);
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
    if (!cinder::lua::protectedCall(state_, 0, LUA_MULTRET)) return;

    for (int i = top + 1; i <= lua_gettop(state_); ++i) {
        cinder::platform::logInfo("[lua] %s\n", luaL_tolstring(state_, i, nullptr));
        lua_pop(state_, 1);
    }
}

void LuaHost::update(float dt) {
    cinder::lua::callGlobal(state_, "__step", static_cast<double>(dt));
}

void LuaHost::render(float alpha) {
    cinder::lua::callGlobal(state_, "__render", static_cast<double>(alpha));
}

void LuaHost::close() {
    if (state_ == nullptr) return;
    scene_.setObserver(nullptr);
    observer_.reset();
    physics_.setObserver(nullptr);
    contacts_.reset();
    lua_close(state_);
    state_ = nullptr;
}

LuaHost::~LuaHost() { close(); }

}
