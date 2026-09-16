#include "dev/Viewport.hpp"

#include "core/Engine.hpp"
#include "dev/Gizmos.hpp"
#include "dev/History.hpp"
#include "dev/Picking.hpp"
#include "dev/PlaySession.hpp"
#include "dev/Selection.hpp"
#include "scene/Node.hpp"
#include "scene/Scene.hpp"

#include <GLFW/glfw3.h>
#include <imgui.h>

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

namespace cinder::dev {
namespace {

constexpr ImGuiButtonFlags ANY_BUTTON = ImGuiButtonFlags_MouseButtonLeft
        | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle;

constexpr int KEYS[] = {GLFW_KEY_1, GLFW_KEY_2, GLFW_KEY_3, GLFW_KEY_X, GLFW_KEY_ESCAPE};

bool held(GLFWwindow* window, int key) {
    return glfwGetKey(window, key) == GLFW_PRESS;
}

float axis(GLFWwindow* window, int negative, int positive) {
    return (held(window, positive) ? 1.0f : 0.0f) - (held(window, negative) ? 1.0f : 0.0f);
}

EditorCamera::Controls readControls(GLFWwindow* window, bool focused, bool grabbing) {
    const ImGuiIO& io = ImGui::GetIO();
    const bool active = ImGui::IsItemActive();
    const bool left = active && !grabbing && ImGui::IsMouseDown(ImGuiMouseButton_Left);
    const bool right = active && ImGui::IsMouseDown(ImGuiMouseButton_Right);
    const bool middle = active && ImGui::IsMouseDown(ImGuiMouseButton_Middle);
    const glm::vec2 drag(io.MouseDelta.x, io.MouseDelta.y);

    EditorCamera::Controls controls;
    controls.looking = right || (left && !io.KeyAlt);
    if (controls.looking) controls.look = drag;
    if (middle || (left && io.KeyAlt)) controls.pan = drag;
    if ((focused || active) && !io.KeyCtrl && !io.KeySuper) {
        controls.move = glm::vec3(axis(window, GLFW_KEY_A, GLFW_KEY_D),
                                  axis(window, GLFW_KEY_Q, GLFW_KEY_E),
                                  axis(window, GLFW_KEY_S, GLFW_KEY_W));
        controls.fast = io.KeyShift;
    }
    if (active || ImGui::IsItemHovered()) controls.scroll = io.MouseWheel;
    return controls;
}

bool clicked(const ImGuiIO& io) {
    const float slop = io.MouseDragThreshold;
    return ImGui::IsItemDeactivated() && ImGui::IsMouseReleased(ImGuiMouseButton_Left) && !io.KeyAlt
            && io.MouseDragMaxDistanceSqr[ImGuiMouseButton_Left] < slop * slop;
}

const char* verb(Tool tool) {
    switch (tool) {
        case Tool::Move: return "Move";
        case Tool::Rotate: return "Rotate";
        case Tool::Scale: return "Scale";
    }
    return "";
}

void toolItem(const char* label, const char* tip, Tool tool, Tool& current) {
    if (ImGui::MenuItem(label, nullptr, current == tool)) current = tool;
    ImGui::SetItemTooltip("%s", tip);
}

}

Viewport::Viewport(PlaySession& session, Selection& selection, History& history, cinder::core::Engine& engine)
    : session_(session), selection_(selection), history_(history), engine_(engine) {}

void Viewport::draw() {
    cinder::platform::Input& input = engine_.input();
    cinder::gfx::Renderer& renderer = engine_.renderer();
    GLFWwindow* window = engine_.window().handle();
    const bool locked = input.cursorLocked();

    ImGuiIO& io = ImGui::GetIO();
    if (locked) io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
    else io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;

    std::array<bool, KEY_COUNT> pressed{};
    for (std::size_t i = 0; i < keys_.size(); ++i) {
        const bool down = held(window, KEYS[i]);
        pressed[i] = down && !keys_[i];
        keys_[i] = down;
    }

    const PlaySession::State state = session_.state();
    const bool playing = state == PlaySession::State::Playing;
    const bool editing = state == PlaySession::State::Edit;
    if (playing && !playing_) ImGui::SetNextWindowFocus();
    playing_ = playing;
    if (!editing) {
        manipulation_.end();
        grabbed_ = false;
    }

    std::vector<cinder::components::Camera*> cameras;
    if (editing) cameras = sceneCameras(engine_.scene());

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    const bool visible = ImGui::Begin(TITLE, nullptr,
                                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse
                                              | ImGuiWindowFlags_MenuBar);
    ImGui::PopStyleVar();

    bool hovered = false;
    bool focused = false;

    if (visible) {
        toolMenu(editing);

        const ImVec2 origin = ImGui::GetCursorScreenPos();
        const ImVec2 available = ImGui::GetContentRegionAvail();
        const int width = std::max(1, static_cast<int>(available.x));
        const int height = std::max(1, static_cast<int>(available.y));
        const ImVec2 size(static_cast<float>(width), static_cast<float>(height));
        const glm::vec2 corner(origin.x, origin.y);
        const glm::vec2 extent(size.x, size.y);

        renderer.setViewportSize(width, height);
        input.setViewportOrigin(origin.x, origin.y);

        ImGui::InvisibleButton("##scene", size, ANY_BUTTON);
        hovered = ImGui::IsItemHovered() || ImGui::IsItemActive();
        focused = ImGui::IsWindowFocused();

        Handle hot = Handle::None;
        if (editing) {
            if (focused && !io.WantTextInput && !io.KeyCtrl && !io.KeySuper && !manipulation_.active()) {
                shortcuts(pressed);
            }

            if (!camera_.seeded()) camera_.seed(cameras.empty() ? nullptr : cameras.back(), extent);
            camera_.setViewSize(extent);
            hot = manipulate(selection_.resolve(engine_.scene()), corner, extent, pressed[KeyCancel]);

            const bool grabbing = grabbed_;
            const bool picking = clicked(io) && !grabbing;
            if (ImGui::IsItemDeactivated()) grabbed_ = false;

            camera_.update(readControls(window, focused, grabbing), io.DeltaTime);
            if (picking) pickAt(glm::vec2(io.MousePos.x, io.MousePos.y) - corner, extent);
        }

        ImDrawList& list = *ImGui::GetWindowDrawList();
        list.AddImage(reinterpret_cast<ImTextureID>(renderer.viewport()), origin,
                      ImVec2(origin.x + size.x, origin.y + size.y));
        if (editing) {
            const glm::mat4& viewProjection = camera_.camera().viewProjection();
            drawFrustums(list, cameras, viewProjection, corner, extent);
            if (cinder::scene::Node* selected = selection_.resolve(engine_.scene())) {
                drawSelection(list, *selected, viewProjection, corner, extent);
                drawGizmo(*selected, hot, corner, extent);
            }
        }
    }
    ImGui::End();

    if (editing && camera_.seeded()) renderer.overrideCamera(camera_.view());
    else renderer.releaseCamera();

    input.setSuppressed(!locked && !focused, !locked && !hovered);
}

void Viewport::toolMenu(bool editing) {
    if (!ImGui::BeginMenuBar()) return;
    ImGui::BeginDisabled(!editing || manipulation_.active());

    toolItem("Move", "Move (1)", Tool::Move, tool_);
    toolItem("Rotate", "Rotate (2)", Tool::Rotate, tool_);
    toolItem("Scale", "Scale (3)", Tool::Scale, tool_);
    ImGui::Separator();

    const char* space = tool_ == Tool::Rotate ? "Gimbal###space"
            : tool_ == Tool::Scale || space_ == Space::Local ? "Local###space"
                                                             : "World###space";
    ImGui::BeginDisabled(tool_ != Tool::Move);
    if (ImGui::MenuItem(space)) space_ = space_ == Space::World ? Space::Local : Space::World;
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled | ImGuiHoveredFlags_ForTooltip)) {
        ImGui::SetTooltip("%s", tool_ == Tool::Move ? "Toggle World/Local (X)"
                                        : tool_ == Tool::Rotate ? "Each ring turns one Euler angle"
                                                                : "Scale is always local");
    }
    ImGui::Separator();

    if (ImGui::GetIO().KeyCtrl) ImGui::TextUnformatted("Snap");
    else ImGui::TextDisabled("Snap");
    ImGui::SetItemTooltip("Hold Ctrl (Cmd on macOS) while dragging to snap");

    ImGui::EndDisabled();
    ImGui::EndMenuBar();
}

void Viewport::shortcuts(const std::array<bool, KEY_COUNT>& pressed) {
    if (pressed[KeyMove]) tool_ = Tool::Move;
    if (pressed[KeyRotate]) tool_ = Tool::Rotate;
    if (pressed[KeyScale]) tool_ = Tool::Scale;
    if (pressed[KeySpace] && tool_ == Tool::Move) space_ = space_ == Space::World ? Space::Local : Space::World;
}

Handle Viewport::manipulate(cinder::scene::Node* selected, glm::vec2 corner, glm::vec2 extent, bool cancel) {
    const ImGuiIO& io = ImGui::GetIO();
    cinder::scene::Scene& scene = engine_.scene();
    const glm::vec2 mouse = glm::vec2(io.MousePos.x, io.MousePos.y) - corner;
    const GizmoView view = gizmoView(camera_.camera(), extent);

    if (manipulation_.active()) {
        cinder::scene::Node* node = scene.byId(*manipulation_.node());
        const std::string label = node != nullptr ? std::string(verb(manipulation_.tool())) + " " + node->name() : "";
        const bool holding = ImGui::IsItemActive() && ImGui::IsMouseDown(ImGuiMouseButton_Left);

        if (cancel) {
            if (manipulation_.cancel(scene)) history_.touch(label, selection_.id());
        } else if (holding && manipulation_.drag(scene, view, mouse, io.KeyCtrl)) {
            history_.touch(label, selection_.id());
        }
        if (!holding) manipulation_.end();
        return manipulation_.active() ? manipulation_.handle() : Handle::None;
    }

    if (selected == nullptr || selected->transform() == nullptr) return Handle::None;
    const std::optional<Gizmo> gizmo = gizmoFor(*selected->transform(), tool_, space_, view);
    if (!gizmo) return Handle::None;

    const bool pressing = ImGui::IsItemActivated() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !io.KeyAlt;
    const bool hovering = ImGui::IsItemHovered() && !ImGui::IsItemActive();
    if (!pressing && !hovering) return Handle::None;

    const Handle handle = hitHandle(shapes(*gizmo, view), view, mouse);
    if (pressing && handle != Handle::None && manipulation_.begin(*selected, *gizmo, handle, view, mouse)) {
        grabbed_ = true;
    }
    return handle;
}

void Viewport::drawGizmo(cinder::scene::Node& node, Handle hot, glm::vec2 corner, glm::vec2 extent) {
    if (node.transform() == nullptr) return;
    const GizmoView view = gizmoView(camera_.camera(), extent);
    const std::optional<Gizmo> gizmo = gizmoFor(*node.transform(), tool_, space_, view);
    if (!gizmo) return;
    drawManipulator(*ImGui::GetWindowDrawList(), shapes(*gizmo, view), hot, view.viewProjection, corner, extent);
}

void Viewport::pickAt(glm::vec2 point, glm::vec2 size) {
    const PickView view{camera_.camera().viewProjection(), point, size};

    if (cinder::scene::Node* node = pick(engine_.scene(), view)) selection_.select(node->id(), true);
    else selection_.clear();
}

}
