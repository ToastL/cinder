#include "dev/Viewport.hpp"

#include "core/Engine.hpp"
#include "dev/Gizmos.hpp"
#include "dev/Picking.hpp"
#include "dev/PlaySession.hpp"
#include "dev/Selection.hpp"
#include "scene/Node.hpp"

#include <imgui.h>

#include <algorithm>
#include <vector>

namespace cinder::dev {
namespace {

constexpr ImGuiButtonFlags ANY_BUTTON = ImGuiButtonFlags_MouseButtonLeft
        | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle;

float axis(ImGuiKey negative, ImGuiKey positive) {
    return (ImGui::IsKeyDown(positive) ? 1.0f : 0.0f) - (ImGui::IsKeyDown(negative) ? 1.0f : 0.0f);
}

EditorCamera::Controls readControls(bool focused) {
    const ImGuiIO& io = ImGui::GetIO();
    const bool active = ImGui::IsItemActive();
    const bool left = active && ImGui::IsMouseDown(ImGuiMouseButton_Left);
    const bool right = active && ImGui::IsMouseDown(ImGuiMouseButton_Right);
    const bool middle = active && ImGui::IsMouseDown(ImGuiMouseButton_Middle);
    const glm::vec2 drag(io.MouseDelta.x, io.MouseDelta.y);

    EditorCamera::Controls controls;
    controls.looking = right || (left && !io.KeyAlt);
    if (controls.looking) controls.look = drag;
    if (middle || (left && io.KeyAlt)) controls.pan = drag;
    if ((focused || active) && !io.KeyCtrl && !io.KeySuper) {
        controls.move = glm::vec3(axis(ImGuiKey_A, ImGuiKey_D), axis(ImGuiKey_Q, ImGuiKey_E),
                                  axis(ImGuiKey_S, ImGuiKey_W));
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

}

Viewport::Viewport(PlaySession& session, Selection& selection, cinder::core::Engine& engine)
    : session_(session), selection_(selection), engine_(engine) {}

void Viewport::draw() {
    cinder::platform::Input& input = engine_.input();
    cinder::gfx::Renderer& renderer = engine_.renderer();
    const bool locked = input.cursorLocked();

    ImGuiIO& io = ImGui::GetIO();
    if (locked) io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
    else io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;

    const PlaySession::State state = session_.state();
    const bool playing = state == PlaySession::State::Playing;
    const bool editing = state == PlaySession::State::Edit;
    if (playing && !playing_) ImGui::SetNextWindowFocus();
    playing_ = playing;

    std::vector<cinder::components::Camera*> cameras;
    if (editing) {
        cameras = perspectiveCameras(engine_.scene());
        if (!camera_.seeded()) camera_.seed(cameras.empty() ? nullptr : cameras.back());
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    const bool visible = ImGui::Begin(TITLE, nullptr,
                                      ImGuiWindowFlags_NoScrollbar
                                              | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::PopStyleVar();

    bool hovered = false;
    bool focused = false;

    if (visible) {
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
        const bool picking = editing && clicked(io);

        if (editing) {
            camera_.setAspect(size.x / size.y);
            camera_.update(readControls(focused), io.DeltaTime);
        }
        if (picking) pickAt(glm::vec2(io.MousePos.x, io.MousePos.y) - corner, extent);

        ImDrawList& list = *ImGui::GetWindowDrawList();
        list.AddImage(reinterpret_cast<ImTextureID>(renderer.viewport()), origin,
                      ImVec2(origin.x + size.x, origin.y + size.y));
        if (editing) {
            const glm::mat4& viewProjection = camera_.camera().viewProjection();
            drawFrustums(list, cameras, viewProjection, corner, extent);
            if (cinder::scene::Node* selected = selection_.resolve(engine_.scene())) {
                drawSelection(list, *selected, viewProjection, renderer.viewProjection2d(), corner, extent);
            }
        }
    }
    ImGui::End();

    if (editing) renderer.overrideCamera3d(camera_.camera());
    else renderer.releaseCamera3d();

    input.setSuppressed(!locked && !focused, !locked && !hovered);
}

void Viewport::pickAt(glm::vec2 point, glm::vec2 size) {
    const glm::vec3 world2d = engine_.renderer().screenToWorld2d(point.x, point.y);
    const PickView view{camera_.camera().viewProjection(), point, size, glm::vec2(world2d)};

    if (cinder::scene::Node* node = pick(engine_.scene(), view)) selection_.select(node->id(), true);
    else selection_.clear();
}

}
