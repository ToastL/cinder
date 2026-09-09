#pragma once

#include "script/ScriptHost.hpp"

#include <lua.hpp>

#include <cstdint>
#include <filesystem>
#include <functional>

namespace cinder::gfx { class Renderer; }
namespace cinder::platform { class Input; }
namespace cinder::scene { class Scene; }

namespace cinder::script {

class LuaHost : public ScriptHost {
public:
    LuaHost(std::filesystem::path source, cinder::scene::Scene& scene, cinder::platform::Input& input,
            cinder::gfx::Renderer& renderer, std::function<void()> quit);
    ~LuaHost() override;

    LuaHost(const LuaHost&) = delete;
    LuaHost& operator=(const LuaHost&) = delete;

    void load() override;
    void poll() override;
    void update(float dt) override;
    void render(float alpha) override;

private:
    void registerScripts();
    void registerApi();
    void loadPrelude();
    void close();

    std::filesystem::path source_;
    cinder::scene::Scene& scene_;
    cinder::platform::Input& input_;
    cinder::gfx::Renderer& renderer_;
    std::function<void()> quit_;

    lua_State* state_ = nullptr;
    std::int64_t lastModified_ = 0;
};

}
