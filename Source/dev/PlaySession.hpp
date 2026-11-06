#pragma once

#include <string>

namespace cinder::core {
class Engine;
class GameLoop;
}

namespace cinder::dev {

class PlaySession {
public:
    enum class State { Edit, Playing, Paused };

    explicit PlaySession(cinder::core::Engine& engine) : engine_(engine) {}

    PlaySession(const PlaySession&) = delete;
    PlaySession& operator=(const PlaySession&) = delete;

    State state() const { return state_; }

    void togglePlay() { request_ = Request::Play; }
    void togglePause() { request_ = Request::Pause; }
    void step() { request_ = Request::Step; }

    void tick(cinder::core::GameLoop& loop);

private:
    enum class Request { None, Play, Pause, Step };

    bool apply();
    void play();
    void stop();

    cinder::core::Engine& engine_;
    std::string snapshot_;
    State state_ = State::Edit;
    Request request_ = Request::None;
};

}
