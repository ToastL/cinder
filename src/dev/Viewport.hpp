#pragma once

#include "dev/EditorCamera.hpp"

namespace cinder::core { class Engine; }

namespace cinder::dev {

class PlaySession;

class Viewport {
public:
    static constexpr const char* TITLE = "Scene";

    Viewport(PlaySession& session, cinder::core::Engine& engine);

    Viewport(const Viewport&) = delete;
    Viewport& operator=(const Viewport&) = delete;

    void draw();

private:
    PlaySession& session_;
    cinder::core::Engine& engine_;
    EditorCamera camera_;
    bool playing_ = false;
};

}
