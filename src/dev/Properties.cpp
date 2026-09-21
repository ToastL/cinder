#include "dev/Properties.hpp"

#include "dev/History.hpp"
#include "dev/PropertyWidgets.hpp"
#include "dev/Selection.hpp"
#include "reflect/Reflect.hpp"
#include "scene/Node.hpp"
#include "scene/Attributes.hpp"
#include "scene/NodeTypes.hpp"
#include "scene/PropValue.hpp"
#include "scene/Scene.hpp"
#include "scene/Transform.hpp"

#include <imgui.h>
#include <imgui_stdlib.h>

#include <cfloat>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace cinder::dev {
namespace {

using namespace propertyWidgets;
using cinder::scene::Node;
using cinder::scene::PropRec;
using cinder::scene::PropSeq;
using cinder::scene::PropValue;

constexpr const char* ADD_POPUP = "Add Attribute";
constexpr const char* KINDS[] = {"Number", "String", "Boolean", "Vector2", "Vector3", "Vector4"};
constexpr int FIRST_VECTOR = 3;

PropValue zeroOf(int kind) {
    if (kind == 1) return PropValue::text("");
    if (kind == 2) return PropValue::flag(false);
    if (kind < FIRST_VECTOR) return PropValue::number(0.0);
    return PropValue::seq(PropSeq(static_cast<std::size_t>(kind - FIRST_VECTOR + 2), PropValue::number(0.0)));
}

bool nodeHeader(Node& node, std::string_view type) {
    std::string name = node.name();
    ImGui::SetNextItemWidth(-FLT_MIN);
    ImGui::InputText("##name", &name);
    const bool renamed = ImGui::IsItemDeactivatedAfterEdit();
    if (renamed) node.setName(std::move(name));

    ImGui::TextDisabled("%.*s  #%d", static_cast<int>(type.size()), type.data(), node.id());
    return renamed;
}

bool classSection(Node& node, std::string_view type) {
    const std::string title = type.empty() ? std::string("Node") : std::string(type);
    if (!ImGui::CollapsingHeader(title.c_str(), ImGuiTreeNodeFlags_DefaultOpen) || !beginRows("class")) {
        return false;
    }
    const bool changed = propRows(node.propList(), node.propTarget());
    ImGui::EndTable();
    return changed;
}

}

Properties::Properties(Selection& selection, History& history, cinder::scene::Scene& scene)
    : selection_(selection), history_(history), scene_(scene) {}

void Properties::draw() {
    Node* node = selection_.resolve(scene_);

    ImGui::SetNextWindowSize(ImVec2(320, 480), ImGuiCond_FirstUseEver);
    const bool visible = ImGui::Begin(TITLE, nullptr, ImGuiWindowFlags_NoFocusOnAppearing);
    if (visible && node == nullptr) ImGui::TextDisabled("Nothing selected");
    if (visible && node != nullptr) inspect(*node);
    ImGui::End();
}

void Properties::inspect(Node& node) {
    const std::string_view type = scene_.types().nameOf(node);
    bool changed = nodeHeader(node, type);

    if (cinder::scene::Transform* transform = node.transform()) {
        if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen) && beginRows("transform")) {
            changed = propRows(cinder::reflect::props<cinder::scene::Transform>(), transform) || changed;
            ImGui::EndTable();
        }
    }

    changed = classSection(node, type) || changed;
    changed = attributes(node) || changed;
    if (changed) history_.touch("Edit " + node.name(), selection_.id());
}

bool Properties::attributes(Node& node) {
    if (!ImGui::CollapsingHeader("Attributes", ImGuiTreeNodeFlags_DefaultOpen)) return false;

    const PropRec values = node.attributes();
    std::optional<std::string> removed;
    bool changed = false;

    if (!values.empty() && ImGui::BeginTable("attributes", 3, ImGuiTableFlags_SizingStretchProp)) {
        const float button = ImGui::GetFrameHeight();
        ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthStretch, LABEL_WEIGHT);
        ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthStretch, 1.0f - LABEL_WEIGHT);
        ImGui::TableSetupColumn("remove", ImGuiTableColumnFlags_WidthFixed, button);

        for (const auto& [name, value] : values) {
            pushId(name);
            row(name);
            if (std::optional<PropValue> next = editAttribute(value)) {
                node.setAttribute(name, std::move(*next));
                changed = true;
            }
            ImGui::TableNextColumn();
            if (ImGui::Button("x", ImVec2(button, button))) removed = name;
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
    if (removed) {
        node.removeAttribute(*removed);
        changed = true;
    }

    if (ImGui::Button("Add Attribute...")) {
        newName_.clear();
        newKind_ = 0;
        ImGui::OpenPopup(ADD_POPUP);
    }
    return addAttribute(node) || changed;
}

bool Properties::addAttribute(Node& node) {
    if (!ImGui::BeginPopup(ADD_POPUP)) return false;

    if (ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere();
    const bool entered = ImGui::InputText("Name", &newName_, ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::Combo("Type", &newKind_, KINDS, IM_ARRAYSIZE(KINDS));

    const bool valid = cinder::scene::isAttributeName(newName_) && node.attribute(newName_) == nullptr;
    ImGui::BeginDisabled(!valid);
    const bool clicked = ImGui::Button("Add");
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::Button("Cancel")) ImGui::CloseCurrentPopup();

    bool added = false;
    if (valid && (entered || clicked)) {
        node.setAttribute(newName_, zeroOf(newKind_));
        ImGui::CloseCurrentPopup();
        added = true;
    }
    ImGui::EndPopup();
    return added;
}

}
