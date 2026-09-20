#pragma once

#include "script/ScriptHost.hpp"

#include <lua.hpp>

#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace cinder::gfx { class Renderer; }
namespace cinder::physics {
class ContactObserver;
class World;
}
namespace cinder::platform { class Input; }
namespace cinder::scene {
class Node;
class Scene;
class SceneObserver;
}

namespace cinder::script {

class LuaHost : public ScriptHost {
public:
    LuaHost(cinder::scene::Scene& scene, cinder::platform::Input& input,
            cinder::gfx::Renderer& renderer, cinder::physics::World& physics,
            std::function<void()> quit);
    ~LuaHost() override;

    LuaHost(const LuaHost&) = delete;
    LuaHost& operator=(const LuaHost&) = delete;

    void boot() override;
    void poll() override;
    void update(float dt) override;
    void render(float alpha) override;

    void eval(const std::string& source);

private:
    struct Watch {
        std::filesystem::path file;
        std::int64_t modified;
    };

    static int scriptRead(lua_State* state);

    void registerScripts();
    void registerApi();
    void loadPrelude();
    void watch(const char* path, const std::filesystem::path& file);
    void reloadScript(const std::string& path);
    int reloadIn(cinder::scene::Node& node, const std::string& path);
    void close();

    cinder::scene::Scene& scene_;
    cinder::platform::Input& input_;
    cinder::gfx::Renderer& renderer_;
    cinder::physics::World& physics_;
    std::function<void()> quit_;

    lua_State* state_ = nullptr;
    std::unique_ptr<cinder::scene::SceneObserver> observer_;
    std::unique_ptr<cinder::physics::ContactObserver> contacts_;
    std::unordered_map<std::string, Watch> watched_;
};

}
