#include "dev/panels/SceneView.hpp"

#include "core/Engine.hpp"
#include "dev/GizmoLines.hpp"
#include "dev/History.hpp"
#include "dev/Picking.hpp"
#include "dev/PlaySession.hpp"
#include "dev/Selection.hpp"
#include "dev/panels/EditorStyle.hpp"
#include "scene/Node.hpp"
#include "scene/Scene.hpp"
#include "ui/core/ElementList.hpp"
#include "ui/framework/Application.hpp"
#include "ui/widgets/Border.hpp"
#include "ui/widgets/BoxPanel.hpp"
#include "ui/widgets/Button.hpp"
#include "ui/widgets/Label.hpp"
#include "ui/widgets/SizeBox.hpp"

#include <glm/geometric.hpp>

#include <algorithm>
#include <cmath>
#include <string>

namespace cinder::dev::panels {

using namespace cinder::ui;
namespace buttons = cinder::platform::buttons;
namespace keys = cinder::platform::keys;

namespace {

const char* verb(Tool tool) {
    switch (tool) {
        case Tool::Move: return "Move";
        case Tool::Rotate: return "Rotate";
        case Tool::Scale: return "Scale";
    }
    return "";
}

bool isDown(std::uint32_t buttons, int button) { return (buttons & (1u << static_cast<unsigned>(button))) != 0; }

float axis(const Application& app, int negative, int positive) {
    return (app.keyHeld(positive) ? 1.0f : 0.0f) - (app.keyHeld(negative) ? 1.0f : 0.0f);
}

bool flyKey(int key) {
    return key == keys::letter('w') || key == keys::letter('a') || key == keys::letter('s') || key == keys::letter('d')
            || key == keys::letter('q') || key == keys::letter('e');
}

Color colorOf(glm::u8vec4 color) { return Color::srgb(color.r, color.g, color.b, color.a); }

}

class SceneView::Client final : public ViewportClient {
public:
    explicit Client(SceneView& view) : view_(view) {}

    void onArrange(const Geometry& geometry) override { view_.arrange(geometry); }
    void tick(const Geometry&, double, float deltaTime) override { view_.tick(deltaTime); }
    int onPaint(const Geometry& geometry, ElementList& list, int layer) override {
        return view_.paint(geometry, list, layer);
    }

    Reply onMouseDown(const Geometry& geometry, const PointerEvent& event) override {
        return view_.mouseDown(geometry, event);
    }
    Reply onMouseDoubleClick(const Geometry& geometry, const PointerEvent& event) override {
        return view_.mouseDown(geometry, event);
    }
    Reply onMouseMove(const Geometry& geometry, const PointerEvent& event) override {
        return view_.mouseMove(geometry, event);
    }
    Reply onMouseUp(const Geometry& geometry, const PointerEvent& event) override {
        return view_.mouseUp(geometry, event);
    }
    Reply onMouseWheel(const Geometry&, const PointerEvent& event) override { return view_.wheel(event); }
    Reply onKeyDown(const Geometry&, const KeyEvent& event) override { return view_.keyDown(event); }
    void onMouseCaptureLost() override { view_.captureLost(); }

private:
    SceneView& view_;
};

SceneView::SceneView(PlaySession& session, Selection& selection, History& history, cinder::core::Engine& engine,
                     Application& app)
    : session_(session), selection_(selection), history_(history), engine_(engine), app_(app) {
    build();
}

bool SceneView::editing() const { return session_.state() == PlaySession::State::Edit; }

void SceneView::build() {
    const auto tool = [this](const char* text, Tool which, const char* tip) {
        return HorizontalBox::slot().autoWidth()
            [make<Button>()
                     .text(text)
                     .buttonStyle([this, which] { return std::string(tool_ == which ? "Button.ToolbarOn" : "Button.Toolbar"); })
                     .isEnabled([this] { return editing() && !manipulation_.active(); })
                     .toolTipText(tip)
                     .onClicked([this, which] {
                         tool_ = which;
                         return Reply::handled();
                     })];
    };
    const auto gap = [](float width) { return HorizontalBox::slot().autoWidth()[make<Spacer>().size(glm::vec2(width, 0.0f))]; };

    widget_ = make<VerticalBox>()
        + VerticalBox::slot().autoHeight()
              [make<Border>()
                       .brush([] { return Application::get().theme().get<Brush>("Brush.TitleBar"); })
                       .padding(Margin(6.0f, 2.0f))
                   [make<HorizontalBox>()
                    + HorizontalBox::slot().autoWidth().vAlign(VAlign::Center)[make<Label>().text("Scene").textStyle("Label.Bold")]
                    + gap(12.0f)
                    + tool("Move", Tool::Move, "Move (1)")
                    + tool("Rotate", Tool::Rotate, "Rotate (2)")
                    + tool("Scale", Tool::Scale, "Scale (3)")
                    + gap(8.0f)
                    + HorizontalBox::slot().autoWidth()
                          [make<Button>()
                                   .text([this] {
                                       if (tool_ == Tool::Rotate) return std::string("Gimbal");
                                       return std::string(tool_ == Tool::Scale || space_ == Space::Local ? "Local" : "World");
                                   })
                                   .buttonStyle("Button.Toolbar")
                                   .isEnabled([this] { return editing() && tool_ == Tool::Move && !manipulation_.active(); })
                                   .toolTipText([this] {
                                       return std::string(tool_ == Tool::Move ? "Toggle World/Local (X)"
                                                          : tool_ == Tool::Rotate ? "Each ring turns one Euler angle"
                                                                                  : "Scale is always local");
                                   })
                                   .onClicked([this] {
                                       space_ = space_ == Space::World ? Space::Local : Space::World;
                                       return Reply::handled();
                                   })]
                    + gap(8.0f)
                    + HorizontalBox::slot().autoWidth().vAlign(VAlign::Center)
                          [make<Label>()
                                   .text("Snap")
                                   .toolTipText("Hold Ctrl (Cmd on macOS) while dragging to snap")
                                   .isEnabled([this] { return editing(); })
                                   .colorAndOpacity(Attribute<Color>([this] {
                                       return app_.theme().color(primaryHeld(app_) ? "Color.ForegroundBright" : "Color.ForegroundDim");
                                   }))]]]
        + VerticalBox::slot().fill(1.0f)[make<Viewport>().assign(viewport_).client(std::make_shared<Client>(*this))];
}

void SceneView::update() {
    app_.setMouseEnabled(!engine_.input().cursorLocked());
    const bool playing = session_.state() == PlaySession::State::Playing;
    if (playing && !playing_) app_.setFocus(viewport_);
    playing_ = playing;
    if (!editing()) {
        manipulation_.end();
        grabbed_ = false;
        picking_ = false;
        hot_ = Handle::None;
    }
}

void SceneView::afterPaint() {
    cinder::gfx::Renderer& renderer = engine_.renderer();
    if (editing() && camera_.seeded()) renderer.overrideCamera(camera_.view());
    else renderer.releaseCamera();

    cinder::platform::Input& input = engine_.input();
    const bool locked = input.cursorLocked();
    const bool focused = viewport_->hasFocus();
    const bool hovered = viewport_->isHovered() || viewport_->hasMouseCapture();
    input.setSuppressed(!locked && !focused, !locked && !hovered);
}

void SceneView::arrange(const Geometry& geometry) {
    origin_ = geometry.position;
    extent_ = glm::max(geometry.size * geometry.scale, glm::vec2(1.0f));
    engine_.renderer().setViewportSize(std::max(1, static_cast<int>(std::lround(extent_.x))),
                                       std::max(1, static_cast<int>(std::lround(extent_.y))));
    engine_.input().setViewportOrigin(origin_.x, origin_.y);
}

void SceneView::tick(float deltaTime) {
    if (!editing()) {
        look_ = glm::vec2(0.0f);
        scroll_ = 0.0f;
        return;
    }
    if (!camera_.seeded()) {
        const std::vector<cinder::components::Camera*> cameras = sceneCameras(engine_.scene());
        camera_.seed(cameras.empty() ? nullptr : cameras.back(), extent_);
    }
    camera_.setViewSize(extent_);

    const bool captured = viewport_->hasMouseCapture();
    const std::uint32_t held = app_.buttons();
    const bool alt = app_.keyHeld(keys::LEFT_ALT) || app_.keyHeld(keys::RIGHT_ALT);
    const bool left = captured && !grabbed_ && isDown(held, buttons::LEFT);
    const bool right = captured && isDown(held, buttons::RIGHT);
    const bool middle = captured && isDown(held, buttons::MIDDLE);

    EditorCamera::Controls controls;
    controls.looking = right || (left && !alt);
    if (controls.looking) controls.look = look_;
    if (middle || (left && alt)) controls.pan = look_;
    const bool control = app_.keyHeld(keys::LEFT_CONTROL) || app_.keyHeld(keys::RIGHT_CONTROL);
    const bool super = app_.keyHeld(keys::LEFT_SUPER) || app_.keyHeld(keys::RIGHT_SUPER);
    if ((viewport_->hasFocus() || captured) && !control && !super) {
        controls.move = glm::vec3(axis(app_, keys::letter('a'), keys::letter('d')),
                                  axis(app_, keys::letter('q'), keys::letter('e')),
                                  axis(app_, keys::letter('s'), keys::letter('w')));
        controls.fast = app_.keyHeld(keys::LEFT_SHIFT) || app_.keyHeld(keys::RIGHT_SHIFT);
    }
    if (captured || viewport_->isHovered()) controls.scroll = scroll_;
    camera_.update(controls, deltaTime);

    look_ = glm::vec2(0.0f);
    scroll_ = 0.0f;
}

GizmoView SceneView::gizmoView() const { return dev::gizmoView(const_cast<EditorCamera&>(camera_).camera(), extent_); }

std::optional<Gizmo> SceneView::gizmo() const {
    cinder::scene::Node* selected = selection_.resolve(engine_.scene());
    if (selected == nullptr || selected->transform() == nullptr) return std::nullopt;
    return gizmoFor(*selected->transform(), tool_, space_, gizmoView());
}

Handle SceneView::handleAt(glm::vec2 point) const {
    if (!editing() || !camera_.seeded()) return Handle::None;
    const std::optional<Gizmo> found = gizmo();
    if (!found) return Handle::None;
    const GizmoView view = gizmoView();
    return hitHandle(shapes(*found, view), view, point);
}

std::string SceneView::label() const {
    if (!manipulation_.active()) return {};
    cinder::scene::Node* node = engine_.scene().byId(*manipulation_.node());
    return node != nullptr ? std::string(verb(manipulation_.tool())) + " " + node->name() : std::string();
}

void SceneView::pickAt(glm::vec2 point) {
    const PickView view{camera_.camera().viewProjection(), point, extent_};
    if (cinder::scene::Node* node = pick(engine_.scene(), view)) selection_.select(node->id(), true);
    else selection_.clear();
}

int SceneView::paint(const Geometry& geometry, ElementList& list, int layer) {
    if (!editing() || !camera_.seeded()) return layer;
    const bool hovering = viewport_->isHovered() && !viewport_->hasMouseCapture();
    hot_ = hovering ? handleAt(geometry.local(app_.cursorPosition())) : Handle::None;
    cinder::scene::Scene& scene = engine_.scene();
    const GizmoCanvas canvas{camera_.camera().viewProjection(), geometry.position, geometry.size * geometry.scale};

    GizmoStrokes strokes;
    frustumStrokes(strokes, sceneCameras(scene), canvas);
    colliderStrokes(strokes, scene, canvas);
    if (cinder::scene::Node* selected = selection_.resolve(scene)) {
        selectionStrokes(strokes, *selected, canvas);
        if (const std::optional<Gizmo> found = gizmo()) {
            const Handle hot = manipulation_.active() ? manipulation_.handle() : hot_;
            manipulatorStrokes(strokes, shapes(*found, gizmoView()), hot, canvas);
        }
    }

    for (const GizmoStroke& stroke : strokes) {
        if (stroke.filled()) list.polygon(layer, stroke.points, colorOf(stroke.color));
        else list.lines(layer, stroke.points, colorOf(stroke.color), stroke.thickness, stroke.closed);
    }
    return layer;
}

Reply SceneView::mouseDown(const Geometry& geometry, const PointerEvent& event) {
    Reply reply = Reply::handled().captureMouse(viewport_).setFocus(viewport_);
    if (!editing() || event.button != buttons::LEFT) return reply;

    pressed_ = event.position;
    travel_ = 0.0f;
    picking_ = !event.alt();
    if (event.alt() || !camera_.seeded()) return reply;

    cinder::scene::Node* selected = selection_.resolve(engine_.scene());
    const std::optional<Gizmo> found = gizmo();
    if (selected == nullptr || !found) return reply;
    const GizmoView view = gizmoView();
    const glm::vec2 point = geometry.local(event.position);
    const Handle handle = hitHandle(shapes(*found, view), view, point);
    if (handle != Handle::None && manipulation_.begin(*selected, *found, handle, view, point)) {
        grabbed_ = true;
        picking_ = false;
    }
    return reply;
}

Reply SceneView::mouseMove(const Geometry& geometry, const PointerEvent& event) {
    const glm::vec2 point = geometry.local(event.position);
    if (!viewport_->hasMouseCapture()) return Reply::unhandled();
    travel_ = std::max(travel_, glm::distance(event.position, pressed_));
    if (manipulation_.active()) {
        if (event.isDown(buttons::LEFT) && manipulation_.drag(engine_.scene(), gizmoView(), point, primaryHeld(app_))) {
            history_.touch(label(), selection_.id());
        }
        return Reply::handled();
    }
    look_ += event.delta;
    return Reply::handled();
}

Reply SceneView::mouseUp(const Geometry& geometry, const PointerEvent& event) {
    if (event.button == buttons::LEFT) {
        if (manipulation_.active()) {
            manipulation_.end();
        } else if (picking_ && editing() && travel_ <= Application::DRAG_THRESHOLD && !event.alt()) {
            pickAt(geometry.local(event.position));
        }
        picking_ = false;
        grabbed_ = false;
    }
    return Reply::handled();
}

Reply SceneView::wheel(const PointerEvent& event) {
    scroll_ += event.wheel.y;
    return Reply::handled();
}

Reply SceneView::keyDown(const KeyEvent& event) {
    if (!editing()) return Reply::unhandled();
    if (event.key == keys::ESCAPE && manipulation_.active()) {
        const std::string name = label();
        if (manipulation_.cancel(engine_.scene())) history_.touch(name, selection_.id());
        manipulation_.end();
        grabbed_ = false;
        return Reply::handled();
    }
    if (event.control() || event.super()) return Reply::unhandled();
    if (!manipulation_.active()) {
        if (event.key == keys::digit(1)) tool_ = Tool::Move;
        else if (event.key == keys::digit(2)) tool_ = Tool::Rotate;
        else if (event.key == keys::digit(3)) tool_ = Tool::Scale;
        else if (event.key == keys::letter('x')) {
            if (tool_ == Tool::Move) space_ = space_ == Space::World ? Space::Local : Space::World;
        } else if (!flyKey(event.key)) {
            return Reply::unhandled();
        }
        return Reply::handled();
    }
    return flyKey(event.key) ? Reply::handled() : Reply::unhandled();
}

void SceneView::captureLost() {
    manipulation_.end();
    picking_ = false;
    grabbed_ = false;
}

}
