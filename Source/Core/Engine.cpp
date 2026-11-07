#include "Core/Engine.hpp"

#include "Nodes/Builtins.hpp"
#include "Physics/Nodes.hpp"
#include "Serial/SceneCodec.hpp"

namespace cinder::Core {
namespace {

Engine* active = nullptr;

}

Engine* Engine::current() { return active; }

Engine::Engine(const ProjectConfig& config)
    : window_(config.title, config.width, config.height),
      input_(window_),
      ctx_(window_),
      renderer_(ctx_, window_) {
    cinder::Nodes::registerBuiltins(types_);
    cinder::Physics::registerNodes(types_);
    physics_.setGravity(config.gravity);
    active = this;
}

bool Engine::running() const { return !quit_ && !window_.shouldClose(); }

bool Engine::minimized() const { return window_.isMinimized(); }

void Engine::loadScene(const std::string& source) { cinder::Serial::SceneCodec::load(source, scene_); }

void Engine::openScene(const std::filesystem::path& path) { cinder::Serial::SceneCodec::loadFromFile(path, scene_); }

void Engine::saveScene(const std::filesystem::path& path) {
    if (path.has_parent_path()) std::filesystem::create_directories(path.parent_path());
    cinder::Serial::SceneCodec::saveToFile(path, scene_);
}

void Engine::update(float dt) {
    scene_.update(dt);
    physics_.step(dt);
    input_.consume();
}

void Engine::render(float alpha) {
    renderer_.beginFrame();
    scene_.render(alpha, renderer_.draws());
    renderer_.drawFrame();
}

Engine::~Engine() {
    if (active == this) active = nullptr;
    ctx_.waitIdle();
    scene_.clear();
}

}
