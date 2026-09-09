#include "core/Engine.hpp"

#include "components/Builtins.hpp"
#include "platform/Assets.hpp"

namespace cinder::core {

Engine::Engine(const GameConfig& config)
    : window_(config.title, config.width, config.height),
      input_(window_),
      ctx_(window_),
      renderer_(ctx_, window_) {
    cinder::components::registerBuiltins(types_);

    script_ = std::make_unique<cinder::script::LuaHost>(
            cinder::platform::resolveAsset(config.script),
            scene_, input_, renderer_, [this] { quit(); });
    script_->load();
}

bool Engine::running() const { return !quit_ && !window_.shouldClose(); }

bool Engine::minimized() const { return window_.isMinimized(); }

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
    scene_.clear();
    script_.reset();
}

}
