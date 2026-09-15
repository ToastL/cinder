#include "core/Engine.hpp"

#include "components/Builtins.hpp"
#include "serial/SceneCodec.hpp"

namespace cinder::core {

Engine::Engine(const ProjectConfig& config) : Engine(config, {}) {}

Engine::Engine(const ProjectConfig& config, const cinder::gfx::OverlayFactory& overlay)
    : window_(config.title, config.width, config.height),
      input_(window_),
      ctx_(window_),
      renderer_(ctx_, window_, overlay) {
    cinder::components::registerBuiltins(types_);

    script_ = std::make_unique<cinder::script::LuaHost>(scene_, input_, renderer_,
                                                        [this] { quit(); });
    script_->boot();
}

bool Engine::running() const { return !quit_ && !window_.shouldClose(); }

bool Engine::minimized() const { return window_.isMinimized(); }

void Engine::reset() {
    scene_.clear();
    script_->boot();
}

void Engine::loadScene(const std::string& source) {
    reset();
    cinder::serial::SceneCodec::load(source, scene_);
}

void Engine::openScene(const std::filesystem::path& path) {
    reset();
    cinder::serial::SceneCodec::loadFromFile(path, scene_);
}

void Engine::saveScene(const std::filesystem::path& path) {
    if (path.has_parent_path()) std::filesystem::create_directories(path.parent_path());
    cinder::serial::SceneCodec::saveToFile(path, scene_);
}

void Engine::beginFrame() { script_->poll(); }

void Engine::update(float dt) {
    script_->update(dt);
    scene_.update(dt);
    input_.consume();
}

void Engine::render(float alpha) {
    renderer_.beginFrame();
    script_->render(alpha);
    scene_.render(alpha, renderer_.draws());
    renderer_.drawFrame();
}

Engine::~Engine() {
    ctx_.waitIdle();
    renderer_.setOverlayDraw(nullptr);
    scene_.clear();
    script_.reset();
}

}
