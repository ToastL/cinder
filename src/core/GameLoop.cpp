#include "core/GameLoop.hpp"

#include "core/Engine.hpp"
#include "platform/Glfw.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>

namespace cinder::core {

GameLoop::GameLoop(int hz) {
    requirePositiveHz(hz, "fixed step rate");
    fixedDt_ = 1.0 / hz;
}

void GameLoop::requirePositiveHz(int hz, const char* what) {
    if (hz <= 0) {
        throw std::runtime_error(std::string(what) + " must be positive, got "
                                 + std::to_string(hz));
    }
}

void GameLoop::tick(Engine& engine) {
    cinder::platform::Glfw::pollEvents();
    engine.beginFrame();

    if (engine.minimized()) {
        cinder::platform::Glfw::waitEvents();
        resetClock();
        return;
    }

    if (last_ < 0.0) last_ = cinder::platform::Glfw::time();

    const double now = cinder::platform::Glfw::time();
    const int steps = advance(now - last_);
    last_ = now;

    for (int i = 0; i < steps; ++i) engine.update(static_cast<float>(fixedDt_));
    engine.render(alpha());
}

int GameLoop::advance(double frameTime) {
    accumulator_ += std::min(frameTime, MAX_FRAME_TIME);
    const int steps = static_cast<int>(accumulator_ / fixedDt_);
    accumulator_ -= steps * fixedDt_;
    return steps;
}

float GameLoop::alpha() const {
    const float value = static_cast<float>(accumulator_ / fixedDt_);
    if (value < 0.0f) return 0.0f;
    return value < 1.0f ? value : std::nextafter(1.0f, 0.0f);
}

void GameLoop::resetClock() {
    last_ = cinder::platform::Glfw::time();
    accumulator_ = 0.0;
}

}
