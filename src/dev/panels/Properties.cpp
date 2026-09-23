#include "dev/panels/Properties.hpp"

#include "dev/History.hpp"
#include "dev/Selection.hpp"
#include "reflect/Reflect.hpp"
#include "scene/Attributes.hpp"
#include "scene/Node.hpp"
#include "scene/NodeTypes.hpp"
#include "scene/PropValue.hpp"
#include "scene/Scene.hpp"
#include "scene/Transform.hpp"
#include "ui/framework/Application.hpp"
#include "ui/widgets/Border.hpp"
#include "ui/widgets/BoxPanel.hpp"
#include "ui/widgets/Button.hpp"
#include "ui/widgets/ColorPicker.hpp"
#include "ui/widgets/ComboBox.hpp"
#include "ui/widgets/ExpandableArea.hpp"
#include "ui/widgets/Label.hpp"
#include "ui/widgets/Menu.hpp"
#include "ui/widgets/ScrollBox.hpp"
#include "ui/widgets/SizeBox.hpp"
#include "ui/widgets/SpinBox.hpp"
#include "ui/widgets/TextField.hpp"
#include "ui/widgets/VectorInputBox.hpp"

#include <glm/trigonometric.hpp>

#include <cfloat>
#include <cstdint>

namespace cinder::dev::panels {

using namespace cinder::ui;
using cinder::reflect::PropDef;
using cinder::reflect::PropHint;
using cinder::reflect::PropList;
using cinder::reflect::PropType;
using cinder::scene::Node;
using cinder::scene::PropRec;
using cinder::scene::PropSeq;
using cinder::scene::PropValue;

namespace {

constexpr float LABEL_FILL = 0.4f;
constexpr float VALUE_FILL = 0.6f;
constexpr double ATTRIBUTE_STEP = 0.1;
const std::vector<std::string> KINDS = {"Number", "String", "Boolean", "Vector2", "Vector3", "Vector4"};

std::optional<double> lowerBound(const PropDef& def) {
    if (def.min() <= -FLT_MAX) return std::nullopt;
    return static_cast<double>(def.min());
}

std::optional<double> upperBound(const PropDef& def) {
    if (def.max() >= FLT_MAX) return std::nullopt;
    return static_cast<double>(def.max());
}

double componentOf(const PropDef& def, void* target, int index) {
    float values[4]{};
    def.read(target, values);
    const double value = values[index];
    return def.hint() == PropHint::Angle ? glm::degrees(value) : value;
}

void writeComponent(const PropDef& def, void* target, int index, double value) {
    float values[4]{};
    def.read(target, values);
    values[index] = def.hint() == PropHint::Angle ? glm::radians(static_cast<float>(value))
                                                  : static_cast<float>(value);
    def.write(target, values);
}

PropValue zeroOf(const std::string& kind) {
    if (kind == "String") return PropValue::text("");
    if (kind == "Boolean") return PropValue::flag(false);
    if (kind == "Vector2") return PropValue::seq(PropSeq(2, PropValue::number(0.0)));
    if (kind == "Vector3") return PropValue::seq(PropSeq(3, PropValue::number(0.0)));
    if (kind == "Vector4") return PropValue::seq(PropSeq(4, PropValue::number(0.0)));
    return PropValue::number(0.0);
}

double numberOf(const PropValue& value) {
    if (value.is<double>()) return value.as<double>();
    if (value.is<std::int64_t>()) return static_cast<double>(value.as<std::int64_t>());
    return 0.0;
}

}

Properties::Properties(Selection& selection, History& history, cinder::scene::Scene& scene, Application& app)
    : selection_(selection), history_(history), scene_(scene), app_(app) {
    host_ = make<Border>().padding(Margin(0.0f));
    widget_ = make<ScrollBox>() + ScrollBox::slot().padding(Margin(6.0f, 4.0f))[host_];
    rebuild();
}

Node* Properties::node() const { return selection_.resolve(scene_); }

void Properties::touched() {
    Node* current = node();
    history_.touch("Edit " + (current != nullptr ? current->name() : std::string("node")), selection_.id());
}

void Properties::update() {
    Node* current = node();
    const std::optional<int> id = current != nullptr ? std::optional<int>(current->id()) : std::nullopt;
    std::vector<std::string> keys;
    if (current != nullptr) {
        for (const auto& [name, value] : current->attributes()) keys.push_back(name);
    }
    if (id == shown_ && keys == keys_) return;
    shown_ = id;
    keys_ = std::move(keys);
    rebuild();
}

void Properties::rebuild() {
    Node* current = node();
    if (current == nullptr) {
        host_->setContent(make<Label>()
                                  .text("Nothing selected")
                                  .colorAndOpacity(Attribute<Color>([] {
                                      return Application::get().theme().color("Color.ForegroundDim");
                                  })));
        return;
    }
    host_->setContent(body(*current));
}

std::shared_ptr<Widget> Properties::row(std::string label, std::shared_ptr<Widget> editor,
                                        std::shared_ptr<Widget> trailing) {
    auto line = make<HorizontalBox>()
            + HorizontalBox::slot().fill(LABEL_FILL).vAlign(VAlign::Center).padding(Margin(0.0f, 0.0f, 6.0f, 0.0f))
                  [make<Label>().text(std::move(label))]
            + HorizontalBox::slot().fill(VALUE_FILL).vAlign(VAlign::Center)[std::move(editor)];
    if (trailing) line + HorizontalBox::slot().autoWidth().vAlign(VAlign::Center).padding(Margin(4.0f, 0.0f, 0.0f, 0.0f))[std::move(trailing)];
    return line;
}

std::shared_ptr<Widget> Properties::editorFor(const PropDef& def, Target target) {
    const PropDef* prop = &def;
    switch (def.type()) {
        case PropType::Bool:
            return make<CheckBox>()
                    .isChecked([prop, target] {
                        void* at = target();
                        return at != nullptr && prop->readBool(at);
                    })
                    .onCheckStateChanged([this, prop, target](bool value) {
                        if (void* at = target()) {
                            prop->writeBool(at, value);
                            touched();
                        }
                    });
        case PropType::String:
            return make<TextBox>()
                    .text([prop, target] {
                        void* at = target();
                        return at != nullptr ? std::string(prop->readText(at)) : std::string();
                    })
                    .onTextCommitted([this, prop, target](const std::string& text, TextCommit) {
                        if (void* at = target()) {
                            prop->writeText(at, text);
                            touched();
                        }
                    });
        case PropType::Enum: {
            std::vector<std::string> options;
            for (const std::string_view option : def.options()) options.emplace_back(option);
            return make<ComboBox>()
                    .options(std::move(options))
                    .selectedOption([prop, target] {
                        void* at = target();
                        return at != nullptr ? std::string(prop->readText(at)) : std::string();
                    })
                    .onSelectionChanged([this, prop, target](const std::string& option) {
                        if (void* at = target()) {
                            prop->writeText(at, option);
                            touched();
                        }
                    });
        }
        case PropType::Int:
        case PropType::Float:
            return make<SpinBox>()
                    .value([prop, target] {
                        void* at = target();
                        return at != nullptr ? componentOf(*prop, at, 0) : 0.0;
                    })
                    .integral(def.type() == PropType::Int)
                    .step(def.step() > 0.0f ? def.step() : 0.1f)
                    .minValue(lowerBound(def))
                    .maxValue(upperBound(def))
                    .units(def.hint() == PropHint::Angle ? "\xC2\xB0" : "")
                    .onValueChanged([this, prop, target](double value) {
                        if (void* at = target()) {
                            writeComponent(*prop, at, 0, value);
                            touched();
                        }
                    });
        default: {
            if (def.hint() == PropHint::Color) {
                const int arity = def.arity();
                return make<HorizontalBox>()
                       + HorizontalBox::slot().autoWidth().vAlign(VAlign::Center)
                             [make<ColorBlock>()
                                      .color([prop, target, arity] {
                                          void* at = target();
                                          float values[4]{0.0f, 0.0f, 0.0f, 1.0f};
                                          if (at != nullptr) prop->read(at, values);
                                          return Color{values[0], values[1], values[2], arity == 4 ? values[3] : 1.0f};
                                      })
                                      .useAlpha(arity == 4)
                                      .opensPicker(true)
                                      .size(glm::vec2(64.0f, 18.0f))
                                      .onColorChanged([this, prop, target, arity](Color color) {
                                          void* at = target();
                                          if (at == nullptr) return;
                                          float values[4]{color.r, color.g, color.b, color.a};
                                          if (arity == 3) {
                                              float current[4]{};
                                              prop->read(at, current);
                                              values[3] = current[3];
                                          }
                                          prop->write(at, values);
                                          touched();
                                      })]
                       + HorizontalBox::slot().fill(1.0f)[make<Spacer>()];
            }
            return make<VectorInputBox>()
                    .components(def.arity())
                    .value([prop, target](int index) {
                        void* at = target();
                        return at != nullptr ? componentOf(*prop, at, index) : 0.0;
                    })
                    .step(def.step() > 0.0f ? def.step() : 0.1f)
                    .units(def.hint() == PropHint::Angle ? "\xC2\xB0" : "")
                    .minValue(lowerBound(def))
                    .maxValue(upperBound(def))
                    .onComponentChanged([this, prop, target](int index, double value) {
                        if (void* at = target()) {
                            writeComponent(*prop, at, index, value);
                            touched();
                        }
                    });
        }
    }
}

std::shared_ptr<Widget> Properties::rows(const PropList& defs, Target target) {
    auto list = make<VerticalBox>();
    for (const PropDef& def : defs) {
        list + VerticalBox::slot().autoHeight().padding(Margin(0.0f, 0.0f, 0.0f, 3.0f))
                       [row(std::string(def.label()), editorFor(def, target))];
    }
    return list;
}

std::shared_ptr<Widget> Properties::attributeEditor(int id, const std::string& name) {
    const auto valueOf = [this, id, name]() -> PropValue {
        const Node* current = scene_.byId(id);
        if (current == nullptr) return PropValue::number(0.0);
        const auto found = current->attributes().find(name);
        return found == current->attributes().end() ? PropValue::number(0.0) : found->second;
    };
    const auto write = [this, id, name](PropValue value) {
        if (Node* current = scene_.byId(id)) {
            current->setAttribute(name, std::move(value));
            touched();
        }
    };
    const PropValue value = valueOf();

    if (!cinder::scene::isAttributeValue(value)) {
        return make<Label>().text("unsupported").colorAndOpacity(Attribute<Color>([] {
            return Application::get().theme().color("Color.ForegroundDim");
        }));
    }
    if (value.is<bool>()) {
        return make<CheckBox>()
                .isChecked([valueOf] {
                    const PropValue current = valueOf();
                    return current.is<bool>() && current.as<bool>();
                })
                .onCheckStateChanged([write](bool flag) { write(PropValue::flag(flag)); });
    }
    if (value.is<std::string>()) {
        return make<TextBox>()
                .text([valueOf] {
                    const PropValue current = valueOf();
                    return current.is<std::string>() ? current.as<std::string>() : std::string();
                })
                .onTextCommitted([write](const std::string& text, TextCommit) { write(PropValue::text(text)); });
    }
    if (!value.is<PropSeq>()) {
        return make<SpinBox>()
                .value([valueOf] { return numberOf(valueOf()); })
                .step(ATTRIBUTE_STEP)
                .onValueChanged([write](double number) { write(PropValue::number(number)); });
    }
    const int count = static_cast<int>(value.as<PropSeq>().size());
    return make<VectorInputBox>()
            .components(count)
            .value([valueOf](int index) {
                const PropValue current = valueOf();
                if (!current.is<PropSeq>()) return 0.0;
                const PropSeq& items = current.as<PropSeq>();
                return static_cast<std::size_t>(index) < items.size()
                        ? numberOf(items[static_cast<std::size_t>(index)])
                        : 0.0;
            })
            .step(ATTRIBUTE_STEP)
            .onComponentChanged([valueOf, write, count](int index, double number) {
                const PropValue current = valueOf();
                PropSeq next;
                for (int i = 0; i < count; ++i) {
                    double value = 0.0;
                    if (current.is<PropSeq>() && static_cast<std::size_t>(i) < current.as<PropSeq>().size()) {
                        value = numberOf(current.as<PropSeq>()[static_cast<std::size_t>(i)]);
                    }
                    next.push_back(PropValue::number(i == index ? number : value));
                }
                write(PropValue::seq(std::move(next)));
            });
}

std::shared_ptr<Widget> Properties::addAttributePopup(int id) {
    return make<Border>()
            .brush([] { return Application::get().theme().get<MenuStyle>("Menu").background; })
            .padding(Margin(10.0f))
            [make<SizeBox>().minDesiredWidth(220.0f)
                 [make<VerticalBox>()
                  + VerticalBox::slot().autoHeight().padding(Margin(0.0f, 0.0f, 0.0f, 6.0f))
                        [row("Name", make<TextBox>()
                                             .text([this] { return newName_; })
                                             .selectAllOnFocus(true)
                                             .onTextChanged([this](const std::string& text) { newName_ = text; }))]
                  + VerticalBox::slot().autoHeight().padding(Margin(0.0f, 0.0f, 0.0f, 10.0f))
                        [row("Type", make<ComboBox>()
                                             .options(KINDS)
                                             .selectedOption([this] { return newKind_; })
                                             .onSelectionChanged([this](const std::string& kind) { newKind_ = kind; }))]
                  + VerticalBox::slot().autoHeight()
                        [make<HorizontalBox>()
                         + HorizontalBox::slot().fill(1.0f)[make<Spacer>()]
                         + HorizontalBox::slot().autoWidth().padding(Margin(0.0f, 0.0f, 6.0f, 0.0f))
                               [make<Button>().text("Cancel").onClicked([this] {
                                   app_.dismissAllPopups();
                                   return Reply::handled();
                               })]
                         + HorizontalBox::slot().autoWidth()
                               [make<Button>()
                                        .text("Add")
                                        .buttonStyle("Button.Primary")
                                        .isEnabled([this, id] {
                                            Node* current = scene_.byId(id);
                                            return current != nullptr && cinder::scene::isAttributeName(newName_)
                                                    && current->attributes().count(newName_) == 0;
                                        })
                                        .onClicked([this, id] {
                                            if (Node* current = scene_.byId(id)) {
                                                touched();
                                                current->setAttribute(newName_, zeroOf(newKind_));
                                                newName_.clear();
                                            }
                                            app_.dismissAllPopups();
                                            return Reply::handled();
                                        })]]]];
}

std::shared_ptr<Widget> Properties::attributesSection(int id) {
    Node* current = scene_.byId(id);
    auto list = make<VerticalBox>();
    if (current != nullptr) {
        for (const auto& [name, value] : current->attributes()) {
            const std::string key = name;
            auto remove = make<Button>()
                    .text("\xC3\x97")
                    .buttonStyle("Button.Small")
                    .toolTipText("Remove " + key)
                    .onClicked([this, id, key] {
                        if (Node* node = scene_.byId(id)) {
                            touched();
                            node->removeAttribute(key);
                        }
                        return Reply::handled();
                    });
            list + VerticalBox::slot().autoHeight().padding(Margin(0.0f, 0.0f, 0.0f, 3.0f))
                           [row(key, attributeEditor(id, key), remove)];
        }
    }
    list + VerticalBox::slot().autoHeight().padding(Margin(0.0f, 4.0f, 0.0f, 0.0f))
                   [make<HorizontalBox>()
                    + HorizontalBox::slot().autoWidth()
                          [make<ComboButton>()
                                   .buttonStyle("Button.Small")
                                   .hasDownArrow(false)
                                   .onGetMenuContent([this, id] { return addAttributePopup(id); })
                                       [make<Label>().text("Add Attribute...")]]
                    + HorizontalBox::slot().fill(1.0f)[make<Spacer>()]];
    return list;
}

std::shared_ptr<Widget> Properties::body(Node& node) {
    const int id = node.id();
    const std::string type(scene_.types().nameOf(node));
    const Target self = [this, id]() -> void* {
        Node* current = scene_.byId(id);
        return current != nullptr ? current->propTarget() : nullptr;
    };
    const Target transform = [this, id]() -> void* {
        Node* current = scene_.byId(id);
        return current != nullptr ? current->transform() : nullptr;
    };

    auto column = make<VerticalBox>()
            + VerticalBox::slot().autoHeight()
                  [make<TextBox>()
                           .text([this, id] {
                               Node* current = scene_.byId(id);
                               return current != nullptr ? current->name() : std::string();
                           })
                           .onTextCommitted([this, id](const std::string& text, TextCommit) {
                               Node* current = scene_.byId(id);
                               if (current == nullptr || text.empty() || text == current->name()) return;
                               touched();
                               current->setName(text);
                           })]
            + VerticalBox::slot().autoHeight().padding(Margin(2.0f, 2.0f, 0.0f, 6.0f))
                  [make<Label>()
                           .text(type + "  #" + std::to_string(id))
                           .textStyle("Label.Small")
                           .colorAndOpacity(Attribute<Color>([] {
                               return Application::get().theme().color("Color.ForegroundDim");
                           }))];

    if (node.transform() != nullptr) {
        column + VerticalBox::slot().autoHeight().padding(Margin(0.0f, 0.0f, 0.0f, 4.0f))
                         [make<ExpandableArea>()
                                  .areaTitle("Transform")
                                  .padding(Margin(6.0f, 6.0f))
                                  .bodyContent(rows(cinder::reflect::props<cinder::scene::Transform>(), transform))];
    }
    column + VerticalBox::slot().autoHeight().padding(Margin(0.0f, 0.0f, 0.0f, 4.0f))
                     [make<ExpandableArea>()
                              .areaTitle(type.empty() ? "Node" : type)
                              .padding(Margin(6.0f, 6.0f))
                              .bodyContent(rows(node.propList(), self))];
    column + VerticalBox::slot().autoHeight()
                     [make<ExpandableArea>()
                              .areaTitle("Attributes")
                              .padding(Margin(6.0f, 6.0f))
                              .bodyContent(attributesSection(id))];
    return column;
}

}
