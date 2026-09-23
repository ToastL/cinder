#pragma once

#include "dev/EditorCamera.hpp"
#include "dev/Manipulator.hpp"
#include "ui/widgets/Viewport.hpp"

#include <memory>

namespace cinder::core { class Engine; }
namespace cinder::scene { class Node; }
namespace cinder::ui { class Application; }

namespace cinder::dev {
class History;
class PlaySession;
class Selection;
}

namespace cinder::dev::panels {

class SceneView {
public:
    SceneView(PlaySession& session, Selection& selection, History& history, cinder::core::Engine& engine,
              cinder::ui::Application& app);

    SceneView(const SceneView&) = delete;
    SceneView& operator=(const SceneView&) = delete;

    const std::shared_ptr<cinder::ui::Widget>& widget() const { return widget_; }
    const std::shared_ptr<cinder::ui::Viewport>& viewport() const { return viewport_; }

    void update();
    void afterPaint();

    Tool tool() const { return tool_; }
    Space space() const { return space_; }
    EditorCamera& camera() { return camera_; }

private:
    class Client;
    friend class Client;

    void arrange(const cinder::ui::Geometry& geometry);
    void tick(float deltaTime);
    int paint(const cinder::ui::Geometry& geometry, cinder::ui::ElementList& list, int layer);
    cinder::ui::Reply mouseDown(const cinder::ui::Geometry& geometry, const cinder::ui::PointerEvent& event);
    cinder::ui::Reply mouseMove(const cinder::ui::Geometry& geometry, const cinder::ui::PointerEvent& event);
    cinder::ui::Reply mouseUp(const cinder::ui::Geometry& geometry, const cinder::ui::PointerEvent& event);
    cinder::ui::Reply wheel(const cinder::ui::PointerEvent& event);
    cinder::ui::Reply keyDown(const cinder::ui::KeyEvent& event);
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
    cinder::core::Engine& engine_;
    cinder::ui::Application& app_;
    std::shared_ptr<cinder::ui::Widget> widget_;
    std::shared_ptr<cinder::ui::Viewport> viewport_;

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
