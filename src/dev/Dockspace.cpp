#include "dev/Dockspace.hpp"

#include "dev/Console.hpp"
#include "dev/Viewport.hpp"

#include <imgui.h>
#include <imgui_internal.h>

namespace cinder::dev {
namespace {

constexpr float CONSOLE_RATIO = 0.28f;

void buildDefaultLayout(ImGuiID dockspace, ImVec2 size) {
    ImGui::DockBuilderAddNode(dockspace, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspace, size);

    ImGuiID scene = dockspace;
    const ImGuiID console = ImGui::DockBuilderSplitNode(scene, ImGuiDir_Down, CONSOLE_RATIO,
                                                        nullptr, &scene);
    ImGui::DockBuilderDockWindow(Viewport::TITLE, scene);
    ImGui::DockBuilderDockWindow(Console::TITLE, console);
    ImGui::DockBuilderFinish(dockspace);
}

}

void drawDockspace() {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const ImGuiID dockspace = ImGui::GetID("Dockspace");
    if (ImGui::DockBuilderGetNode(dockspace) == nullptr) {
        buildDefaultLayout(dockspace, viewport->WorkSize);
    }
    ImGui::DockSpaceOverViewport(dockspace, viewport);
}

}
