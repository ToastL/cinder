#pragma once

#include "core/ProjectConfig.hpp"
#include "gfx/Overlay.hpp"
#include "gfx/Renderer.hpp"
#include "gfx/vk/VkCtx.hpp"
#include "physics/World.hpp"
#include "platform/Input.hpp"
#include "platform/Window.hpp"
#include "scene/NodeTypes.hpp"
#include "scene/Scene.hpp"
#include "script/LuaHost.hpp"

#include <filesystem>
#include <memory>
#include <string>

namespace cinder::core {

class Engine {
public:
    explicit Engine(const ProjectConfig& config);
    Engine(const ProjectConfig& config, const cinder::gfx::OverlayFactory& overlay);
    ~Engine();

    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    bool running() const;
    bool minimized() const;

    void loadScene(const std::string& source);
    void openScene(const std::filesystem::path& path);
    void saveScene(const std::filesystem::path& path);

    void beginFrame();
    void update(float dt);
    void render(float alpha);

    void quit() { quit_ = true; }
    bool quitRequested() const { return quit_; }
    void clearQuitRequest() { quit_ = false; }

    cinder::platform::Window& window() { return window_; }
    cinder::platform::Input& input() { return input_; }
    cinder::gfx::Renderer& renderer() { return renderer_; }
    cinder::scene::Scene& scene() { return scene_; }
    cinder::physics::World& physics() { return physics_; }
    cinder::scene::NodeTypes& types() { return types_; }
    cinder::script::LuaHost& script() { return *script_; }

private:
    void reset();

    cinder::platform::Window window_;
    cinder::platform::Input input_;
    cinder::gfx::vk::VkCtx ctx_;
    cinder::gfx::Renderer renderer_;
    cinder::scene::NodeTypes types_;
    cinder::scene::Scene scene_{types_};
    cinder::physics::World physics_{scene_};
    std::unique_ptr<cinder::script::LuaHost> script_;
    bool quit_ = false;
};

}
