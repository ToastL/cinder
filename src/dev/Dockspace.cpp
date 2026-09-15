#include "dev/Dockspace.hpp"

#include "dev/Console.hpp"
#include "dev/Explorer.hpp"
#include "dev/Properties.hpp"
#include "dev/Viewport.hpp"

#include <imgui.h>
#include <imgui_internal.h>

namespace cinder::dev {
namespace {

constexpr float PROPERTIES_RATIO = 0.26f;
constexpr float CONSOLE_RATIO = 0.28f;
constexpr float EXPLORER_RATIO = 0.24f;

void buildDefaultLayout(ImGuiID dockspace, ImVec2 size) {
    ImGui::DockBuilderAddNode(dockspace, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspace, size);

    ImGuiID scene = dockspace;
    const ImGuiID properties = ImGui::DockBuilderSplitNode(scene, ImGuiDir_Right, PROPERTIES_RATIO,
                                                          nullptr, &scene);
    const ImGuiID console = ImGui::DockBuilderSplitNode(scene, ImGuiDir_Down, CONSOLE_RATIO,
                                                        nullptr, &scene);
    const ImGuiID explorer = ImGui::DockBuilderSplitNode(scene, ImGuiDir_Left, EXPLORER_RATIO,
                                                          nullptr, &scene);
    ImGui::DockBuilderDockWindow(Viewport::TITLE, scene);
    ImGui::DockBuilderDockWindow(Explorer::TITLE, explorer);
    ImGui::DockBuilderDockWindow(Properties::TITLE, properties);
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
