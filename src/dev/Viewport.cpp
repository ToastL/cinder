#include "dev/Viewport.hpp"

#include "core/Engine.hpp"
#include "dev/PlaySession.hpp"

#include <imgui.h>

#include <algorithm>

namespace cinder::dev {
namespace {

constexpr ImGuiButtonFlags ANY_BUTTON = ImGuiButtonFlags_MouseButtonLeft
        | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle;

}

Viewport::Viewport(PlaySession& session, cinder::core::Engine& engine)
    : session_(session), engine_(engine) {}

void Viewport::draw() {
    cinder::platform::Input& input = engine_.input();
    cinder::gfx::Renderer& renderer = engine_.renderer();
    const bool locked = input.cursorLocked();

    ImGuiIO& io = ImGui::GetIO();
    if (locked) io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
    else io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;

    const bool playing = session_.state() == PlaySession::State::Playing;
    if (playing && !playing_) ImGui::SetNextWindowFocus();
    playing_ = playing;

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

        renderer.setViewportSize(width, height);
        input.setViewportOrigin(origin.x, origin.y);

        ImGui::InvisibleButton("##scene", size, ANY_BUTTON);
        hovered = ImGui::IsItemHovered() || ImGui::IsItemActive();
        focused = ImGui::IsWindowFocused();

        ImGui::GetWindowDrawList()->AddImage(reinterpret_cast<ImTextureID>(renderer.viewport()),
                                             origin,
                                             ImVec2(origin.x + size.x, origin.y + size.y));
    }
    ImGui::End();

    input.setSuppressed(!locked && !focused, !locked && !hovered);
}

}
