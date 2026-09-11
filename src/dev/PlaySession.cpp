#include "dev/PlaySession.hpp"

#include "core/Engine.hpp"
#include "core/GameLoop.hpp"
#include "platform/Log.hpp"
#include "serial/SceneCodec.hpp"

#include <utility>

namespace cinder::dev {

void PlaySession::tick(cinder::core::GameLoop& loop) {
    if (engine_.quitRequested()) {
        engine_.clearQuitRequest();
        if (state_ != State::Edit) stop();
    }

    const bool stepping = apply();

    if (state_ == State::Playing) loop.tick(engine_);
    else if (stepping) loop.step(engine_);
    else loop.idle(engine_);
}

bool PlaySession::apply() {
    switch (std::exchange(request_, Request::None)) {
        case Request::None:
            return false;
        case Request::Play:
            if (state_ == State::Edit) play();
            else stop();
            return false;
        case Request::Pause:
            if (state_ == State::Playing) state_ = State::Paused;
            else if (state_ == State::Paused) state_ = State::Playing;
            return false;
        case Request::Step:
            if (state_ == State::Edit) return false;
            state_ = State::Paused;
            return true;
    }
    return false;
}

void PlaySession::play() {
    snapshot_ = cinder::serial::SceneCodec::save(engine_.scene());
    engine_.loadScene(snapshot_);
    state_ = State::Playing;
    cinder::platform::logInfo("[editor] play\n");
}

void PlaySession::stop() {
    engine_.input().setCursorLocked(false);
    engine_.loadScene(snapshot_);
    state_ = State::Edit;
    cinder::platform::logInfo("[editor] stop\n");
}

}
