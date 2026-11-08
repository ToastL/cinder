#pragma once

#include "Dev/EditorCamera.hpp"
#include "Dev/Manipulator.hpp"
#include "UI/Widgets/Viewport.hpp"

#include <memory>

namespace cinder::Core { class Engine; }
namespace cinder::UI { class Application; }

namespace cinder::Dev {
class History;
class PlaySession;
class Selection;
}

namespace cinder::Dev::Panels {

class SceneView {
public:
    SceneView(PlaySession& session, Selection& selection, History& history, cinder::Core::Engine& engine,
              cinder::UI::Application& app);

    SceneView(const SceneView&) = delete;
    SceneView& operator=(const SceneView&) = delete;

    const std::shared_ptr<cinder::UI::Widget>& widget() const { return widget_; }
    const std::shared_ptr<cinder::UI::Viewport>& viewport() const { return viewport_; }

    void update();
    void afterPaint();
    void sceneChanged();

    Tool tool() const { return tool_; }
    Space space() const { return space_; }
    EditorCamera& camera() { return camera_; }

private:
    class Client;
    friend class Client;

    void arrange(const cinder::UI::Geometry& geometry);
    void tick(float deltaTime);
    int paint(const cinder::UI::Geometry& geometry, cinder::UI::ElementList& list, int layer);
    cinder::UI::Reply mouseDown(const cinder::UI::Geometry& geometry, const cinder::UI::PointerEvent& event);
    cinder::UI::Reply mouseMove(const cinder::UI::Geometry& geometry, const cinder::UI::PointerEvent& event);
    cinder::UI::Reply mouseUp(const cinder::UI::Geometry& geometry, const cinder::UI::PointerEvent& event);
    cinder::UI::Reply wheel(const cinder::UI::PointerEvent& event);
    cinder::UI::Reply keyDown(const cinder::UI::KeyEvent& event);
    void captureLost();

    bool editing() const;
    void build();
    GizmoView gizmoView() const;
    std::optional<Gizmo> gizmo() const;
    Handle handleAt(glm::vec2 point) const;
    void pickAt(glm::vec2 point);
    std::string label() const;

    PlaySession& session_;
    Selection& selection_;
    History& history_;
    cinder::Core::Engine& engine_;
    cinder::UI::Application& app_;
    std::shared_ptr<cinder::UI::Widget> widget_;
    std::shared_ptr<cinder::UI::Viewport> viewport_;

    EditorCamera camera_;
    Manipulation manipulation_;
    Tool tool_ = Tool::Move;
    Space space_ = Space::World;
    Handle hot_ = Handle::None;

    glm::vec2 origin_{0.0f};
    glm::vec2 extent_{1.0f};
    glm::vec2 look_{0.0f};
    float scroll_ = 0.0f;
    glm::vec2 pressed_{0.0f};
    float travel_ = 0.0f;
    bool picking_ = false;
    bool grabbed_ = false;
    bool playing_ = false;
};

}
