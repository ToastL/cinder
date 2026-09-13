#include "dev/Dockspace.hpp"

#include "dev/Console.hpp"
#include "dev/Hierarchy.hpp"
#include "dev/Inspector.hpp"
#include "dev/Viewport.hpp"

#include <imgui.h>
#include <imgui_internal.h>

namespace cinder::dev {
namespace {

constexpr float INSPECTOR_RATIO = 0.26f;
constexpr float CONSOLE_RATIO = 0.28f;
constexpr float HIERARCHY_RATIO = 0.24f;

void buildDefaultLayout(ImGuiID dockspace, ImVec2 size) {
    ImGui::DockBuilderAddNode(dockspace, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspace, size);

    ImGuiID scene = dockspace;
    const ImGuiID inspector = ImGui::DockBuilderSplitNode(scene, ImGuiDir_Right, INSPECTOR_RATIO,
                                                          nullptr, &scene);
    const ImGuiID console = ImGui::DockBuilderSplitNode(scene, ImGuiDir_Down, CONSOLE_RATIO,
                                                        nullptr, &scene);
    const ImGuiID hierarchy = ImGui::DockBuilderSplitNode(scene, ImGuiDir_Left, HIERARCHY_RATIO,
                                                          nullptr, &scene);
    ImGui::DockBuilderDockWindow(Viewport::TITLE, scene);
    ImGui::DockBuilderDockWindow(Hierarchy::TITLE, hierarchy);
    ImGui::DockBuilderDockWindow(Inspector::TITLE, inspector);
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
