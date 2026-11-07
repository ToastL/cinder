#pragma once

#include "Core/ProjectConfig.hpp"
#include "Gfx/Renderer.hpp"
#include "Gfx/RHI/Backend.hpp"
#include "Physics/World.hpp"
#include "Platform/Input.hpp"
#include "Platform/Window.hpp"
#include "Scene/NodeTypes.hpp"
#include "Scene/Scene.hpp"

#include <filesystem>
#include <memory>
#include <string>

namespace cinder::Core {

class Engine {
public:
    explicit Engine(const ProjectConfig& config);
    static Engine* current();
    ~Engine();

    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    bool running() const;
    bool minimized() const;

    void loadScene(const std::string& source);
    void openScene(const std::filesystem::path& path);
    void saveScene(const std::filesystem::path& path);

    void update(float dt);
    void render(float alpha);

    void quit() { quit_ = true; }
    bool quitRequested() const { return quit_; }
    void clearQuitRequest() { quit_ = false; }

    cinder::Platform::Window& window() { return window_; }
    cinder::Platform::Input& input() { return input_; }
    cinder::Gfx::Renderer& renderer() { return renderer_; }
    cinder::Scene::Scene& scene() { return scene_; }
    cinder::Physics::World& physics() { return physics_; }
    cinder::Scene::NodeTypes& types() { return types_; }

private:

    cinder::Platform::Window window_;
    cinder::Platform::Input input_;
    cinder::Gfx::RHI::Ctx ctx_;
    cinder::Gfx::Renderer renderer_;
    cinder::Scene::NodeTypes types_;
    cinder::Scene::Scene scene_{types_};
    cinder::Physics::World physics_{scene_};
    bool quit_ = false;
};

}
