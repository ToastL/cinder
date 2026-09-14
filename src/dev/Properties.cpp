#include "dev/Properties.hpp"

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
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace cinder::dev {
namespace {

using cinder::reflect::PropDef;
using cinder::reflect::PropHint;
using cinder::reflect::PropList;
using cinder::reflect::PropType;
using cinder::scene::Node;
using cinder::scene::PropRec;
using cinder::scene::PropSeq;
using cinder::scene::PropValue;

constexpr float LABEL_WEIGHT = 0.4f;
constexpr float ATTRIBUTE_STEP = 0.1f;
constexpr ImGuiSliderFlags DRAG = ImGuiSliderFlags_NoRoundToFormat;
constexpr ImGuiColorEditFlags COLOR = ImGuiColorEditFlags_Float;
constexpr const char* ADD_POPUP = "Add Attribute";
constexpr const char* KINDS[] = {"Number", "String", "Boolean", "Vector2", "Vector3", "Vector4"};
constexpr int FIRST_VECTOR = 3;

struct Range {
    float min;
    float max;
};

Range dragRange(const PropDef& def) {
    if (def.min() <= -FLT_MAX && def.max() >= FLT_MAX) return {0.0f, 0.0f};
    return {def.min(), def.max()};
}

void pushId(std::string_view id) { ImGui::PushID(id.data(), id.data() + id.size()); }

bool beginRows(const char* id) {
    if (!ImGui::BeginTable(id, 2, ImGuiTableFlags_SizingStretchProp)) return false;
    ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthStretch, LABEL_WEIGHT);
    ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthStretch, 1.0f - LABEL_WEIGHT);
    return true;
}

void row(std::string_view label) {
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label.data(), label.data() + label.size());
    ImGui::TableNextColumn();
    ImGui::SetNextItemWidth(-FLT_MIN);
}

bool editText(std::string& text) {
    ImGui::InputText("##value", &text);
    return ImGui::IsItemDeactivatedAfterEdit();
}

bool editVector(const PropDef& def, float* values) {
    const int arity = def.arity();
    if (def.hint() == PropHint::Color && arity == 3) return ImGui::ColorEdit3("##value", values, COLOR);
    if (def.hint() == PropHint::Color && arity == 4) return ImGui::ColorEdit4("##value", values, COLOR);

    const Range range = dragRange(def);
    return ImGui::DragScalarN("##value", ImGuiDataType_Float, values, arity, def.step(), &range.min,
                              &range.max, "%.3f", DRAG);
}

void editProp(const PropDef& def, void* target) {
    switch (def.type()) {
        case PropType::Bool: {
            bool value = def.readBool(target);
            if (ImGui::Checkbox("##value", &value)) def.writeBool(target, value);
            break;
        }
        case PropType::String: {
            std::string text(def.readText(target));
            if (editText(text)) def.writeText(target, text);
            break;
        }
        case PropType::Enum: {
            const std::string current(def.readText(target));
            if (!ImGui::BeginCombo("##value", current.c_str())) break;
            for (std::string_view option : def.options()) {
                const std::string name(option);
                if (ImGui::Selectable(name.c_str(), name == current)) def.writeText(target, name);
            }
            ImGui::EndCombo();
            break;
        }
        case PropType::Int: {
            float raw = 0.0f;
            def.read(target, &raw);
            int value = static_cast<int>(raw);
            if (ImGui::DragInt("##value", &value, def.step())) {
                const float next = static_cast<float>(value);
                def.write(target, &next);
            }
            break;
        }
        case PropType::Float: {
            float value = 0.0f;
            def.read(target, &value);
            const Range range = dragRange(def);
            if (ImGui::DragFloat("##value", &value, def.step(), range.min, range.max, "%.3f", DRAG)) {
                def.write(target, &value);
            }
            break;
        }
        default: {
            float values[4]{};
            def.read(target, values);
            if (editVector(def, values)) def.write(target, values);
            break;
        }
    }
}

double numberOf(const PropValue& value) {
    return value.is<double>() ? value.as<double>() : static_cast<double>(value.as<std::int64_t>());
}

std::optional<PropValue> editAttribute(const PropValue& value) {
    if (!cinder::scene::isAttributeValue(value)) {
        ImGui::AlignTextToFramePadding();
        ImGui::TextDisabled("unsupported");
        return std::nullopt;
    }

    if (value.is<bool>()) {
        bool flag = value.as<bool>();
        if (ImGui::Checkbox("##value", &flag)) return PropValue::flag(flag);
        return std::nullopt;
    }

    if (value.is<std::string>()) {
        std::string text = value.as<std::string>();
        if (editText(text)) return PropValue::text(std::move(text));
        return std::nullopt;
    }

    if (!value.is<PropSeq>()) {
        double number = numberOf(value);
        if (!ImGui::DragScalar("##value", ImGuiDataType_Double, &number, ATTRIBUTE_STEP, nullptr, nullptr,
                               "%g", DRAG)) {
            return std::nullopt;
        }
        return PropValue::number(number);
    }

    const PropSeq& items = value.as<PropSeq>();
    const int count = static_cast<int>(items.size());
    double values[4]{};
    for (int i = 0; i < count; ++i) values[i] = numberOf(items[static_cast<std::size_t>(i)]);

    if (!ImGui::DragScalarN("##value", ImGuiDataType_Double, values, count, ATTRIBUTE_STEP, nullptr,
                            nullptr, "%g", DRAG)) {
        return std::nullopt;
    }

    PropSeq next;
    for (int i = 0; i < count; ++i) next.push_back(PropValue::number(values[i]));
    return PropValue::seq(std::move(next));
}

PropValue zeroOf(int kind) {
    if (kind == 1) return PropValue::text("");
    if (kind == 2) return PropValue::flag(false);
    if (kind < FIRST_VECTOR) return PropValue::number(0.0);
    return PropValue::seq(PropSeq(static_cast<std::size_t>(kind - FIRST_VECTOR + 2), PropValue::number(0.0)));
}

void propRows(const PropList& defs, void* target) {
    for (const PropDef& def : defs) {
        pushId(def.name());
        row(def.label());
        editProp(def, target);
        ImGui::PopID();
    }
}

void nodeHeader(Node& node, std::string_view type) {
    std::string name = node.name();
    ImGui::SetNextItemWidth(-FLT_MIN);
    ImGui::InputText("##name", &name);
    if (ImGui::IsItemDeactivatedAfterEdit()) node.setName(std::move(name));

    ImGui::TextDisabled("%.*s  #%d", static_cast<int>(type.size()), type.data(), node.id());
}

void classSection(Node& node, std::string_view type) {
    const std::string title = type.empty() ? std::string("Node") : std::string(type);
    if (ImGui::CollapsingHeader(title.c_str(), ImGuiTreeNodeFlags_DefaultOpen) && beginRows("class")) {
        propRows(node.propList(), node.propTarget());
        ImGui::EndTable();
    }
}

}

Properties::Properties(Selection& selection, cinder::scene::Scene& scene)
    : selection_(selection), scene_(scene) {}

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
    nodeHeader(node, type);

    if (cinder::scene::Transform* transform = node.transform()) {
        if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen) && beginRows("transform")) {
            propRows(cinder::reflect::props<cinder::scene::Transform>(), transform);
            ImGui::EndTable();
        }
    }

    classSection(node, type);
    attributes(node);
}

void Properties::attributes(Node& node) {
    if (!ImGui::CollapsingHeader("Attributes", ImGuiTreeNodeFlags_DefaultOpen)) return;

    const PropRec values = node.attributes();
    std::optional<std::string> removed;

    if (!values.empty() && ImGui::BeginTable("attributes", 3, ImGuiTableFlags_SizingStretchProp)) {
        const float button = ImGui::GetFrameHeight();
        ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthStretch, LABEL_WEIGHT);
        ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthStretch, 1.0f - LABEL_WEIGHT);
        ImGui::TableSetupColumn("remove", ImGuiTableColumnFlags_WidthFixed, button);

        for (const auto& [name, value] : values) {
            pushId(name);
            row(name);
            if (std::optional<PropValue> next = editAttribute(value)) node.setAttribute(name, std::move(*next));
            ImGui::TableNextColumn();
            if (ImGui::Button("x", ImVec2(button, button))) removed = name;
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
    if (removed) node.removeAttribute(*removed);

    if (ImGui::Button("Add Attribute...")) {
        newName_.clear();
        newKind_ = 0;
        ImGui::OpenPopup(ADD_POPUP);
    }
    addAttribute(node);
}

void Properties::addAttribute(Node& node) {
    if (!ImGui::BeginPopup(ADD_POPUP)) return;

    if (ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere();
    const bool entered = ImGui::InputText("Name", &newName_, ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::Combo("Type", &newKind_, KINDS, IM_ARRAYSIZE(KINDS));

    const bool valid = cinder::scene::isAttributeName(newName_) && node.attribute(newName_) == nullptr;
    ImGui::BeginDisabled(!valid);
    const bool clicked = ImGui::Button("Add");
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::Button("Cancel")) ImGui::CloseCurrentPopup();

    if (valid && (entered || clicked)) {
        node.setAttribute(newName_, zeroOf(newKind_));
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}

}
