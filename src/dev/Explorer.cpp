#include "dev/Explorer.hpp"

#include "dev/Selection.hpp"
#include "scene/Node.hpp"
#include "scene/NodeTypes.hpp"
#include "scene/Scene.hpp"
#include "script/Script.hpp"

#include <imgui.h>
#include <imgui_stdlib.h>

#include <cfloat>
#include <cstdint>
#include <filesystem>
#include <string_view>
#include <utility>
#include <vector>

namespace cinder::dev {
namespace {

using cinder::scene::Node;

constexpr const char* DRAG_PAYLOAD = "cinder.node";
constexpr ImGuiTreeNodeFlags ROW = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick
        | ImGuiTreeNodeFlags_NavLeftJumpsToParent;

bool containsIgnoreCase(std::string_view text, std::string_view needle) {
    if (needle.size() > text.size()) return false;
    for (std::size_t i = 0; i + needle.size() <= text.size(); ++i) {
        std::size_t j = 0;
        while (j < needle.size()
               && cinder::reflect::lowerAscii(text[i + j]) == cinder::reflect::lowerAscii(needle[j])) {
            ++j;
        }
        if (j == needle.size()) return true;
    }
    return false;
}

std::string detailOf(const Node& node, std::string_view type) {
    const auto* script = dynamic_cast<const cinder::script::Script*>(&node);
    if (script != nullptr && !script->file().empty()) {
        return std::filesystem::path(script->file()).filename().string();
    }
    return std::string(type);
}

}

Explorer::Explorer(Selection& selection, cinder::scene::Scene& scene)
    : selection_(selection), scene_(scene) {}

void Explorer::draw() {
    ImGui::SetNextWindowSize(ImVec2(260, 480), ImGuiCond_FirstUseEver);
    if (ImGui::Begin(TITLE, nullptr, ImGuiWindowFlags_NoFocusOnAppearing)) {
        Node* selected = selection_.resolve(scene_);

        if (ImGui::Button("+")) ImGui::OpenPopup("insert");
        if (ImGui::BeginPopup("insert")) {
            insertMenu(selected);
            ImGui::EndPopup();
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(-FLT_MIN);
        ImGui::InputTextWithHint("##filter", "Filter", &filter_);

        ImGui::BeginChild("tree");
        const std::vector<Node*> roots = scene_.roots();
        for (Node* root : roots) row(*root);
        if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)
            && !ImGui::IsAnyItemHovered()) {
            selection_.clear();
        }
        ImGui::EndChild();

        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(DRAG_PAYLOAD)) {
                action_ = Action::Reparent;
                target_ = *static_cast<const int*>(payload->Data);
                destination_.reset();
            }
            ImGui::EndDragDropTarget();
        }

        const bool removing = ImGui::IsKeyPressed(ImGuiKey_Delete, false)
                || ImGui::IsKeyPressed(ImGuiKey_Backspace, false);
        if (selected != nullptr && removing && !ImGui::GetIO().WantTextInput
            && ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) {
            action_ = Action::Delete;
            target_ = selected->id();
        }
    }
    ImGui::End();
    apply();
}

void Explorer::row(Node& node) {
    if (node.destroyed()) return;

    const std::string_view type = scene_.types().nameOf(node);
    const std::vector<Node*> children = node.children();
    const bool filtering = !filter_.empty();
    const bool shown = !filtering || containsIgnoreCase(node.name(), filter_)
            || containsIgnoreCase(type, filter_);

    bool open = false;
    if (shown) {
        const bool leaf = filtering || children.empty();
        ImGuiTreeNodeFlags flags = ROW;
        if (leaf) flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        const bool selected = selection_.selected(node.id());
        if (selected) flags |= ImGuiTreeNodeFlags_Selected;

        const bool dimmed = !selected && !node.enabledInHierarchy();
        if (dimmed) ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
        open = ImGui::TreeNodeEx(reinterpret_cast<void*>(static_cast<std::intptr_t>(node.id())), flags, "%s",
                                 node.name().c_str());
        if (dimmed) ImGui::PopStyleColor();

        if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && !ImGui::IsItemToggledOpen()) {
            selection_.select(node.id());
        }
        dragAndDrop(node);
        contextMenu(node);

        const std::string detail = detailOf(node, type);
        if (detail != node.name()) {
            ImGui::SameLine();
            ImGui::TextDisabled("%s", detail.c_str());
        }
        open = open && !leaf;
    }

    if (filtering) {
        for (Node* child : children) row(*child);
        return;
    }
    if (!open) return;
    for (Node* child : children) row(*child);
    ImGui::TreePop();
}

void Explorer::dragAndDrop(Node& node) {
    if (ImGui::BeginDragDropSource()) {
        const int id = node.id();
        ImGui::SetDragDropPayload(DRAG_PAYLOAD, &id, sizeof(id));
        ImGui::TextUnformatted(node.name().c_str());
        ImGui::EndDragDropSource();
    }
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(DRAG_PAYLOAD)) {
            action_ = Action::Reparent;
            target_ = *static_cast<const int*>(payload->Data);
            destination_ = node.id();
        }
        ImGui::EndDragDropTarget();
    }
}

void Explorer::contextMenu(Node& node) {
    if (!ImGui::BeginPopupContextItem()) return;
    if (ImGui::BeginMenu("Insert")) {
        insertMenu(&node);
        ImGui::EndMenu();
    }
    if (ImGui::MenuItem("Duplicate")) {
        action_ = Action::Duplicate;
        target_ = node.id();
    }
    if (ImGui::MenuItem("Delete")) {
        action_ = Action::Delete;
        target_ = node.id();
    }
    ImGui::EndPopup();
}

void Explorer::insertMenu(Node* parent) {
    for (const auto& entry : scene_.types().registered()) {
        if (!ImGui::MenuItem(entry.name.c_str())) continue;
        action_ = Action::Insert;
        className_ = entry.name;
        if (parent != nullptr) destination_ = parent->id();
        else destination_.reset();
    }
}

void Explorer::apply() {
    const Action action = std::exchange(action_, Action::None);
    Node* target = scene_.byId(target_);
    Node* destination = destination_ ? scene_.byId(*destination_) : nullptr;

    switch (action) {
        case Action::None:
            break;
        case Action::Insert:
            if (Node* node = scene_.create(className_, destination)) selection_.select(node->id());
            break;
        case Action::Duplicate:
            if (target == nullptr) break;
            if (Node* copy = scene_.clone(*target, target->parent())) selection_.select(copy->id());
            break;
        case Action::Delete:
            if (target != nullptr) scene_.destroyNow(target);
            break;
        case Action::Reparent:
            if (target != nullptr && target != destination && !target->isAncestorOf(destination)) {
                target->setParent(destination);
            }
            break;
    }
}

}
