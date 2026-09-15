#pragma once

#include "dev/EditorCamera.hpp"

namespace cinder::core { class Engine; }

namespace cinder::dev {

class PlaySession;
class Selection;

class Viewport {
public:
    static constexpr const char* TITLE = "Scene";

    Viewport(PlaySession& session, Selection& selection, cinder::core::Engine& engine);

    Viewport(const Viewport&) = delete;
    Viewport& operator=(const Viewport&) = delete;

    void draw();

private:
    void pickAt(glm::vec2 point, glm::vec2 size);

    PlaySession& session_;
    Selection& selection_;
    cinder::core::Engine& engine_;
    EditorCamera camera_;
    bool playing_ = false;
};

}
