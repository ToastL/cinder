#include "dev/Hierarchy.hpp"

#include "dev/Selection.hpp"
#include "scene/Actor.hpp"
#include "scene/Scene.hpp"

#include <imgui.h>

#include <cstdint>

namespace cinder::dev {
namespace {

constexpr ImGuiTreeNodeFlags NODE = ImGuiTreeNodeFlags_OpenOnArrow
        | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth
        | ImGuiTreeNodeFlags_NavLeftJumpsToParent;

}

Hierarchy::Hierarchy(Selection& selection, cinder::scene::Scene& scene)
    : selection_(selection), scene_(scene) {}

void Hierarchy::draw() {
    ImGui::SetNextWindowSize(ImVec2(260, 480), ImGuiCond_FirstUseEver);
    if (ImGui::Begin(TITLE, nullptr, ImGuiWindowFlags_NoFocusOnAppearing)) {
        for (cinder::scene::Actor* root : scene_.roots()) node(*root);

        if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)
            && !ImGui::IsAnyItemHovered()) {
            selection_.clear();
        }
    }
    ImGui::End();
}

void Hierarchy::node(cinder::scene::Actor& actor) {
    if (actor.destroyed()) return;

    const bool leaf = actor.children().empty();
    ImGuiTreeNodeFlags flags = NODE;
    if (leaf) flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    if (selection_.selected(actor.id())) flags |= ImGuiTreeNodeFlags_Selected;

    const bool inactive = !actor.active();
    if (inactive) ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
    const bool open = ImGui::TreeNodeEx(reinterpret_cast<void*>(static_cast<std::intptr_t>(actor.id())),
                                        flags, "%s", actor.name().c_str());
    if (inactive) ImGui::PopStyleColor();

    if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && !ImGui::IsItemToggledOpen()) {
        selection_.select(actor.id());
    }

    if (!open || leaf) return;
    for (cinder::scene::Actor* child : actor.children()) node(*child);
    ImGui::TreePop();
}

}
