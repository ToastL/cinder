#pragma once

#include "dev/EditorCamera.hpp"
#include "dev/Manipulator.hpp"

#include <array>

namespace cinder::core { class Engine; }
namespace cinder::scene { class Node; }

namespace cinder::dev {

class History;
class PlaySession;
class Selection;

class Viewport {
public:
    static constexpr const char* TITLE = "Scene";

    Viewport(PlaySession& session, Selection& selection, History& history, cinder::core::Engine& engine);

    Viewport(const Viewport&) = delete;
    Viewport& operator=(const Viewport&) = delete;

    void draw();

private:
    enum Key { KeyMove, KeyRotate, KeyScale, KeySpace, KeyCancel, KEY_COUNT };

    void toolMenu(bool editing);
    void shortcuts(const std::array<bool, KEY_COUNT>& pressed);
    Handle manipulate(cinder::scene::Node* selected, glm::vec2 corner, glm::vec2 extent, bool cancel);
    void drawGizmo(cinder::scene::Node& node, Handle hot, glm::vec2 corner, glm::vec2 extent);
    void pickAt(glm::vec2 point, glm::vec2 size);

    PlaySession& session_;
    Selection& selection_;
    History& history_;
    cinder::core::Engine& engine_;
    EditorCamera camera_;
    Manipulation manipulation_;
    Tool tool_ = Tool::Move;
    Space space_ = Space::World;
    std::array<bool, KEY_COUNT> keys_{};
    bool grabbed_ = false;
    bool playing_ = false;
};

}
