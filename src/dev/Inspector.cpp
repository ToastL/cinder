#include "dev/Inspector.hpp"

#include "dev/Selection.hpp"
#include "reflect/Reflect.hpp"
#include "scene/Actor.hpp"
#include "scene/Component.hpp"
#include "scene/Components.hpp"
#include "scene/PropValue.hpp"
#include "scene/Scene.hpp"
#include "scene/Transform.hpp"

#include <imgui.h>
#include <imgui_stdlib.h>

#include <cfloat>
#include <cmath>
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
using cinder::scene::Actor;
using cinder::scene::Component;
using cinder::scene::PropBag;
using cinder::scene::PropRec;
using cinder::scene::PropSeq;
using cinder::scene::PropValue;

constexpr float LABEL_WEIGHT = 0.4f;
constexpr float BAG_STEP = 0.1f;
constexpr ImGuiSliderFlags DRAG = ImGuiSliderFlags_NoRoundToFormat;
constexpr ImGuiColorEditFlags COLOR = ImGuiColorEditFlags_Float;

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

void note(const char* format, std::size_t count) {
    ImGui::AlignTextToFramePadding();
    ImGui::TextDisabled(format, count);
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

std::optional<PropValue> editNumbers(const PropSeq& items) {
    const std::size_t count = items.size();
    double values[4]{};
    bool numeric = count > 0 && count <= 4;
    for (std::size_t i = 0; numeric && i < count; ++i) {
        if (items[i].is<double>()) values[i] = items[i].as<double>();
        else if (items[i].is<std::int64_t>()) values[i] = static_cast<double>(items[i].as<std::int64_t>());
        else numeric = false;
    }

    if (!numeric) {
        note("%zu items", count);
        return std::nullopt;
    }
    if (!ImGui::DragScalarN("##value", ImGuiDataType_Double, values, static_cast<int>(count), BAG_STEP,
                            nullptr, nullptr, "%g", DRAG)) {
        return std::nullopt;
    }

    PropSeq next;
    for (std::size_t i = 0; i < count; ++i) {
        next.push_back(items[i].is<std::int64_t>() ? PropValue::integer(std::llround(values[i]))
                                                   : PropValue::number(values[i]));
    }
    return PropValue::seq(std::move(next));
}

std::optional<PropValue> editValue(const PropValue& value) {
    if (value.is<bool>()) {
        bool flag = value.as<bool>();
        if (ImGui::Checkbox("##value", &flag)) return PropValue::flag(flag);
    } else if (value.is<std::int64_t>()) {
        std::int64_t number = value.as<std::int64_t>();
        if (ImGui::DragScalar("##value", ImGuiDataType_S64, &number, BAG_STEP)) {
            return PropValue::integer(number);
        }
    } else if (value.is<double>()) {
        double number = value.as<double>();
        if (ImGui::DragScalar("##value", ImGuiDataType_Double, &number, BAG_STEP, nullptr, nullptr, "%g",
                              DRAG)) {
            return PropValue::number(number);
        }
    } else if (value.is<std::string>()) {
        std::string text = value.as<std::string>();
        if (editText(text)) return PropValue::text(std::move(text));
    } else if (value.is<PropSeq>()) {
        return editNumbers(value.as<PropSeq>());
    } else {
        note("%zu fields", value.as<PropRec>().size());
    }
    return std::nullopt;
}

void propRows(const PropList& defs, void* target) {
    for (const PropDef& def : defs) {
        pushId(def.name());
        row(def.label());
        editProp(def, target);
        ImGui::PopID();
    }
}

void bagRows(PropBag& bag) {
    const PropRec values = bag.readBag();

    ImGui::PushID("bag");
    for (const auto& [key, value] : values) {
        pushId(key);
        row(cinder::reflect::deriveLabel(key));
        if (std::optional<PropValue> next = editValue(value)) {
            bag.patchBag(PropRec{{key, std::move(*next)}});
        }
        ImGui::PopID();
    }
    ImGui::PopID();
}

void actorHeader(Actor& actor) {
    bool active = actor.activeSelf();
    if (ImGui::Checkbox("##active", &active)) actor.setActive(active);
    ImGui::SameLine();

    std::string name = actor.name();
    ImGui::SetNextItemWidth(-FLT_MIN);
    ImGui::InputText("##name", &name);
    if (ImGui::IsItemDeactivatedAfterEdit()) actor.setName(std::move(name));

    ImGui::TextDisabled("Actor %d", actor.id());
}

void componentSection(Component& component, std::string_view type, int index) {
    ImGui::PushID(index);
    const std::string title = type.empty() ? std::string("Component") : std::string(type);
    if (ImGui::CollapsingHeader(title.c_str(), ImGuiTreeNodeFlags_DefaultOpen) && beginRows("rows")) {
        propRows(component.propList(), component.propTarget());
        if (auto* bag = dynamic_cast<PropBag*>(&component)) bagRows(*bag);
        ImGui::EndTable();
    }
    ImGui::PopID();
}

void inspect(Actor& actor, const cinder::scene::Components& types) {
    actorHeader(actor);

    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen) && beginRows("transform")) {
        propRows(cinder::reflect::props<cinder::scene::Transform>(), &actor.transform());
        ImGui::EndTable();
    }

    const auto& components = actor.components();
    for (std::size_t i = 0; i < components.size(); ++i) {
        Component& component = *components[i];
        componentSection(component, types.nameOf(component), static_cast<int>(i));
    }
}

}

Inspector::Inspector(Selection& selection, cinder::scene::Scene& scene)
    : selection_(selection), scene_(scene) {}

void Inspector::draw() {
    Actor* actor = selection_.resolve(scene_);

    ImGui::SetNextWindowSize(ImVec2(320, 480), ImGuiCond_FirstUseEver);
    const bool visible = ImGui::Begin(TITLE, nullptr, ImGuiWindowFlags_NoFocusOnAppearing);
    if (visible && actor == nullptr) ImGui::TextDisabled("Nothing selected");
    if (visible && actor != nullptr) inspect(*actor, scene_.types());
    ImGui::End();
}

}
