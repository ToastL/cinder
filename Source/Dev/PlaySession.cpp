#include "Dev/PlaySession.hpp"

#include "Core/Engine.hpp"
#include "Core/GameLoop.hpp"
#include "Platform/Log.hpp"
#include "Serial/SceneCodec.hpp"

#include <utility>

namespace cinder::Dev {

void PlaySession::tick(cinder::Core::GameLoop& loop) {
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
    snapshot_ = cinder::Serial::SceneCodec::save(engine_.scene());
    engine_.loadScene(snapshot_);
    state_ = State::Playing;
    cinder::Platform::logInfo("[editor] play\n");
}

void PlaySession::stop() {
    engine_.input().setCursorLocked(false);
    engine_.loadScene(snapshot_);
    state_ = State::Edit;
    cinder::Platform::logInfo("[editor] stop\n");
}

}
