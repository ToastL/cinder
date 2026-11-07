#include "Dev/Panels/Details.hpp"

#include "Dev/History.hpp"
#include "Dev/Selection.hpp"
#include "Dev/Panels/EditorStyle.hpp"
#include "Reflect/Reflect.hpp"
#include "Scene/Attributes.hpp"
#include "Scene/Actor.hpp"
#include "Scene/ClassRegistry.hpp"
#include "Scene/Component.hpp"
#include "Scene/SceneComponent.hpp"
#include "Scene/PropValue.hpp"
#include "Scene/Scene.hpp"
#include "Scene/Transform.hpp"
#include "UI/Framework/Application.hpp"
#include "UI/Widgets/Border.hpp"
#include "UI/Widgets/BoxPanel.hpp"
#include "UI/Widgets/Button.hpp"
#include "UI/Widgets/ColorPicker.hpp"
#include "UI/Widgets/ComboBox.hpp"
#include "UI/Widgets/ExpandableArea.hpp"
#include "UI/Widgets/Label.hpp"
#include "UI/Widgets/ScrollBox.hpp"
#include "UI/Widgets/SizeBox.hpp"
#include "UI/Widgets/SpinBox.hpp"
#include "UI/Widgets/Menu.hpp"
#include "UI/Widgets/TextField.hpp"
#include "UI/Widgets/VectorInputBox.hpp"

#include <glm/trigonometric.hpp>

#include <cfloat>
#include <cstdint>
#include <functional>

namespace cinder::Dev::Panels {

using namespace cinder::UI;
using cinder::Reflect::PropDef;
using cinder::Reflect::PropHint;
using cinder::Reflect::PropList;
using cinder::Reflect::PropType;
using cinder::Scene::Actor;
using cinder::Scene::Component;
using cinder::Scene::SceneComponent;
using cinder::Scene::PropRec;
using cinder::Scene::PropSeq;
using cinder::Scene::PropValue;

namespace {

constexpr float LABEL_FILL = 0.4f;
constexpr float VALUE_FILL = 0.6f;
constexpr float DEFAULT_STEP = 0.1f;
constexpr const char* DEGREES = "\xC2\xB0";
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

Details::Details(Selection& selection, History& history, cinder::Scene::Scene& scene, Application& app)
    : selection_(selection), history_(history), scene_(scene), app_(app) {
    host_ = make<Border>().padding(Margin(0.0f));
    widget_ = make<ScrollBox>() + ScrollBox::slot().padding(Margin(6.0f, 4.0f))[host_];
    rebuild();
}

Actor* Details::actor() const { return selection_.actor(scene_); }

cinder::Scene::Object* Details::editing() const {
    cinder::Scene::Object* selected = selection_.resolve(scene_);
    if (auto* chosen = dynamic_cast<Actor*>(selected)) {
        if (SceneComponent* root = chosen->rootComponent()) return root;
    }
    return selected;
}

void* Details::targetOf(int id, bool transform) const {
    cinder::Scene::Object* object = scene_.byId(id);
    if (object == nullptr || object->destroyed()) return nullptr;
    if (!transform) return object->propTarget();
    auto* component = dynamic_cast<Component*>(object);
    SceneComponent* placed = component != nullptr ? component->asSceneComponent() : nullptr;
    return placed != nullptr ? &placed->transform() : nullptr;
}

void Details::touched() {
    Actor* current = actor();
    history_.touch("Edit " + (current != nullptr ? current->name() : std::string("actor")), selection_.id());
}

void Details::update() {
    Actor* current = actor();
    const std::optional<int> id = selection_.resolve(scene_) != nullptr ? selection_.id() : std::nullopt;
    std::vector<std::string> keys;
    std::vector<int> components;
    if (current != nullptr) {
        for (const auto& [name, value] : current->attributes()) keys.push_back(name);
        for (const Component* component : current->components()) {
            if (!component->destroyed()) components.push_back(component->id());
        }
    }
    if (id == shown_ && keys == keys_ && components == components_) return;
    shown_ = id;
    keys_ = std::move(keys);
    components_ = std::move(components);
    rebuild();
}

void Details::rebuild() {
    selection_.pick().cancel();
    Actor* current = actor();
    if (current == nullptr) {
        host_->setContent(make<Label>()
                                  .text("Nothing selected")
                                  .colorAndOpacity(themeColor("Color.ForegroundDim")));
        return;
    }
    host_->setContent(body(*current));
}

std::shared_ptr<Widget> Details::row(std::string label, std::shared_ptr<Widget> editor,
                                        std::shared_ptr<Widget> trailing) {
    auto line = make<HorizontalBox>()
            + HorizontalBox::slot().fill(LABEL_FILL).vAlign(VAlign::Center).padding(Margin(0.0f, 0.0f, 6.0f, 0.0f))
                  [make<Label>().text(std::move(label))]
            + HorizontalBox::slot().fill(VALUE_FILL).vAlign(VAlign::Center)[std::move(editor)];
    if (trailing) line + HorizontalBox::slot().autoWidth().vAlign(VAlign::Center).padding(Margin(4.0f, 0.0f, 0.0f, 0.0f))[std::move(trailing)];
    return line;
}

std::shared_ptr<Widget> Details::colorEditorFor(const PropDef& def, Target target) {
    const PropDef* prop = &def;
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
                          .onColorChanged(writing(target, [prop, arity](void* at, Color color) {
                              float values[4]{color.r, color.g, color.b, color.a};
                              if (arity == 3) {
                                  float current[4]{};
                                  prop->read(at, current);
                                  values[3] = current[3];
                              }
                              prop->write(at, values);
                          }))]
           + HorizontalBox::slot().fill(1.0f)[make<Spacer>()];
}

cinder::Scene::Object* Details::referenced(const PropDef& def, Target target) const {
    void* at = target();
    if (at == nullptr) return nullptr;
    cinder::Scene::Object* object = scene_.byId(def.readInt(at));
    if (object == nullptr || object->destroyed() || !def.accepts(object)) return nullptr;
    return object;
}

std::string Details::referenceLabel(const cinder::Scene::Object* object) const {
    if (object == nullptr) return "None";
    if (const auto* component = dynamic_cast<const Component*>(object)) {
        if (component->owner() != nullptr) return component->owner()->name() + " \xE2\x80\xBA " + component->name();
    }
    return object->name();
}

std::shared_ptr<Widget> Details::refEditorFor(const PropDef& def, Target target) {
    const PropDef* prop = &def;
    const auto key = std::make_shared<char>();
    const auto assign = writing(target, [prop](void* at, int id) { prop->writeInt(at, id); });

    const auto current = [prop, target] {
        void* at = target();
        return at != nullptr ? prop->readInt(at) : 0;
    };
    auto menu = [this, prop, assign, current] {
        MenuBuilder builder;
        builder.check("None", [assign] { assign(0); }, [current] { return current() == 0; });
        std::vector<const cinder::Scene::Object*> found;
        const std::function<void(const Actor&)> walk = [&](const Actor& actor) {
            if (actor.destroyed()) return;
            if (prop->accepts(static_cast<const cinder::Scene::Object*>(&actor))) found.push_back(&actor);
            for (const Component* component : actor.components()) {
                if (!component->destroyed() && prop->accepts(static_cast<const cinder::Scene::Object*>(component))) {
                    found.push_back(component);
                }
            }
            for (const Actor* child : actor.attachedActors()) walk(*child);
        };
        for (const Actor* root : scene_.roots()) walk(*root);
        if (!found.empty()) builder.separator();
        for (const cinder::Scene::Object* object : found) {
            const int id = object->id();
            builder.check(referenceLabel(object), [assign, id] { assign(id); }, [current, id] { return current() == id; });
        }
        return builder.build();
    };

    auto pick = [this, prop, key, assign] {
        ReferencePick& pick = selection_.pick();
        if (pick.armedBy(key.get())) {
            pick.cancel();
            return Reply::handled();
        }
        pick.arm(key.get(), [prop, assign](cinder::Scene::Object& object) {
            if (!prop->accepts(static_cast<const cinder::Scene::Object*>(&object))) return false;
            assign(object.id());
            return true;
        });
        return Reply::handled();
    };

    auto browse = [this, prop, target] {
        if (cinder::Scene::Object* object = referenced(*prop, target)) selection_.select(object->id(), true);
        return Reply::handled();
    };

    return make<HorizontalBox>()
           + HorizontalBox::slot().fill(1.0f).vAlign(VAlign::Center)
                 [make<ComboButton>().onGetMenuContent(menu)
                          [make<Label>().text([this, prop, target] { return referenceLabel(referenced(*prop, target)); })]]
           + HorizontalBox::slot().autoWidth().vAlign(VAlign::Center).padding(Margin(4.0f, 0.0f, 0.0f, 0.0f))
                 [make<Button>()
                          .text("Pick")
                          .buttonStyle([this, key] {
                              return selection_.pick().armedBy(key.get()) ? std::string("Button.Primary")
                                                                          : std::string("Button");
                          })
                          .toolTipText("Pick in the Scene view or the Outliner")
                          .onClicked(pick)]
           + HorizontalBox::slot().autoWidth().vAlign(VAlign::Center).padding(Margin(4.0f, 0.0f, 0.0f, 0.0f))
                 [make<Button>()
                          .text("Go")
                          .isEnabled([this, prop, target] { return referenced(*prop, target) != nullptr; })
                          .toolTipText("Select the referenced object")
                          .onClicked(browse)];
}

std::shared_ptr<Widget> Details::editorFor(const PropDef& def, Target target) {
    const PropDef* prop = &def;
    const auto text = [prop, target] {
        void* at = target();
        return at != nullptr ? std::string(prop->readText(at)) : std::string();
    };
    switch (def.type()) {
        case PropType::Bool:
            return make<CheckBox>()
                    .isChecked([prop, target] {
                        void* at = target();
                        return at != nullptr && prop->readBool(at);
                    })
                    .onCheckStateChanged(writing(target, [prop](void* at, bool value) { prop->writeBool(at, value); }));
        case PropType::String:
            return make<TextBox>().text(text).onTextCommitted(
                    [write = writing(target, [prop](void* at, const std::string& value) { prop->writeText(at, value); })](
                            const std::string& value, TextCommit how) {
                        if (how != TextCommit::Cleared) write(value);
                    });
        case PropType::Enum: {
            std::vector<std::string> options;
            for (const std::string_view option : def.options()) options.emplace_back(option);
            return make<ComboBox>()
                    .options(std::move(options))
                    .selectedOption(text)
                    .onSelectionChanged(
                            writing(target, [prop](void* at, const std::string& option) { prop->writeText(at, option); }));
        }
        case PropType::Ref:
            return refEditorFor(def, target);
        case PropType::Int:
        case PropType::Float:
            return make<SpinBox>()
                    .value([prop, target] {
                        void* at = target();
                        return at != nullptr ? componentOf(*prop, at, 0) : 0.0;
                    })
                    .integral(def.type() == PropType::Int)
                    .step(def.step() > 0.0f ? def.step() : DEFAULT_STEP)
                    .minValue(lowerBound(def))
                    .maxValue(upperBound(def))
                    .units(def.hint() == PropHint::Angle ? DEGREES : "")
                    .onValueChanged(writing(target, [prop](void* at, double value) { writeComponent(*prop, at, 0, value); }));
        default:
            if (def.hint() == PropHint::Color) return colorEditorFor(def, target);
            return make<VectorInputBox>()
                    .components(def.arity())
                    .value([prop, target](int index) {
                        void* at = target();
                        return at != nullptr ? componentOf(*prop, at, index) : 0.0;
                    })
                    .step(def.step() > 0.0f ? def.step() : DEFAULT_STEP)
                    .units(def.hint() == PropHint::Angle ? DEGREES : "")
                    .minValue(lowerBound(def))
                    .maxValue(upperBound(def))
                    .onComponentChanged(writing(target, [prop](void* at, int index, double value) {
                        writeComponent(*prop, at, index, value);
                    }));
    }
}

std::shared_ptr<Widget> Details::rows(const PropList& defs, Target target) {
    auto list = make<VerticalBox>();
    for (const PropDef& def : defs) {
        list + VerticalBox::slot().autoHeight().padding(Margin(0.0f, 0.0f, 0.0f, 3.0f))
                       [row(std::string(def.label()), editorFor(def, target))];
    }
    return list;
}

std::shared_ptr<Widget> Details::attributeEditor(int id, const std::string& name) {
    const auto valueOf = [this, id, name]() -> PropValue {
        const Actor* current = scene_.actor(id);
        if (current == nullptr) return PropValue::number(0.0);
        const auto found = current->attributes().find(name);
        return found == current->attributes().end() ? PropValue::number(0.0) : found->second;
    };
    const auto write = [this, id, name](PropValue value) {
        if (Actor* current = scene_.actor(id)) {
            current->setAttribute(name, std::move(value));
            touched();
        }
    };
    const PropValue value = valueOf();

    if (!cinder::Scene::isAttributeValue(value)) {
        return make<Label>().text("unsupported").colorAndOpacity(themeColor("Color.ForegroundDim"));
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
                .onTextCommitted([write](const std::string& text, TextCommit how) {
                    if (how != TextCommit::Cleared) write(PropValue::text(text));
                });
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

std::shared_ptr<Widget> Details::addAttributePopup(int id) {
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
                                            Actor* current = scene_.actor(id);
                                            return current != nullptr && cinder::Scene::isAttributeName(newName_)
                                                    && current->attributes().count(newName_) == 0;
                                        })
                                        .onClicked([this, id] {
                                            if (Actor* current = scene_.actor(id)) {
                                                touched();
                                                current->setAttribute(newName_, zeroOf(newKind_));
                                                newName_.clear();
                                            }
                                            app_.dismissAllPopups();
                                            return Reply::handled();
                                        })]]]];
}

std::shared_ptr<Widget> Details::attributesSection(int id) {
    Actor* current = scene_.actor(id);
    auto list = make<VerticalBox>();
    if (current != nullptr) {
        for (const auto& [name, value] : current->attributes()) {
            const std::string key = name;
            auto remove = make<Button>()
                    .text("\xC3\x97")
                    .buttonStyle("Button.Small")
                    .toolTipText("Remove " + key)
                    .onClicked([this, id, key] {
                        if (Actor* node = scene_.actor(id)) {
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

std::shared_ptr<Widget> Details::componentsSection(Actor& owner) {
    const int actorId = owner.id();
    auto list = make<VerticalBox>();

    const auto entry = [&](const auto& self, Component& component, int depth) -> void {
        const int id = component.id();
        const std::string type(scene_.types().nameOf(component));
        const std::string label = component.name() == type ? type : component.name() + "  (" + type + ")";
        auto line = make<HorizontalBox>()
                + HorizontalBox::slot().fill(1.0f).padding(Margin(static_cast<float>(depth) * 12.0f, 0.0f, 0.0f, 0.0f))
                      [make<Button>()
                               .text(label)
                               .hAlign(HAlign::Left)
                               .buttonStyle([this, id] {
                                   return selection_.selected(id) ? std::string("Button.Primary") : std::string("Button");
                               })
                               .onClicked([this, id] {
                                   selection_.select(id);
                                   return Reply::handled();
                               })];
        if (!component.isDefault()) {
            line + HorizontalBox::slot().autoWidth().vAlign(VAlign::Center).padding(Margin(4.0f, 0.0f, 0.0f, 0.0f))
                       [make<Button>()
                                .text("x")
                                .toolTipText("Remove this component")
                                .onClicked([this, id] {
                                    removeComponent(id);
                                    return Reply::handled();
                                })];
        }
        list + VerticalBox::slot().autoHeight().padding(Margin(0.0f, 1.0f))[line];

        if (SceneComponent* placed = component.asSceneComponent()) {
            for (SceneComponent* child : placed->attachChildren()) {
                if (!child->destroyed()) self(self, *child, depth + 1);
            }
        }
    };

    SceneComponent* root = owner.rootComponent();
    if (root != nullptr) entry(entry, *root, 0);
    for (Component* component : owner.components()) {
        const SceneComponent* placed = component->asSceneComponent();
        if (component->destroyed() || component == root || placed == nullptr || placed->attachParent() != nullptr) continue;
        entry(entry, *component, root != nullptr ? 1 : 0);
    }
    for (Component* component : owner.components()) {
        if (!component->destroyed() && component->asSceneComponent() == nullptr) entry(entry, *component, 0);
    }

    list + VerticalBox::slot().autoHeight().padding(Margin(0.0f, 4.0f, 0.0f, 0.0f))
               [make<HorizontalBox>()
                + HorizontalBox::slot().autoWidth()
                      [make<ComboButton>()
                               .onGetMenuContent([this, actorId] { return addComponentMenu(actorId); })
                               [make<Label>().text("Add Component")]]
                + HorizontalBox::slot().fill(1.0f)[make<Spacer>()]];
    return list;
}

std::shared_ptr<Widget> Details::addComponentMenu(int actorId) {
    MenuBuilder menu;
    for (const auto& entry : scene_.types().registered()) {
        if (!entry.factory || entry.kind != cinder::Scene::ClassKind::Component) continue;
        const std::string name = entry.name;
        menu.entry(name, [this, actorId, name] { addComponent(actorId, name); });
    }
    return menu.build();
}

void Details::addComponent(int actorId, const std::string& className) {
    Actor* owner = scene_.actor(actorId);
    if (owner == nullptr) return;
    history_.touch("Add " + className, selection_.id());
    SceneComponent* under = nullptr;
    if (auto* selected = dynamic_cast<Component*>(selection_.resolve(scene_))) {
        if (selected->owner() == owner) under = selected->asSceneComponent();
    }
    if (Component* added = scene_.addComponent(*owner, className, under)) selection_.select(added->id());
}

void Details::removeComponent(int id) {
    Component* component = scene_.component(id);
    if (component == nullptr || component->isDefault()) return;
    Actor* owner = component->owner();
    history_.touch("Remove " + component->name(), selection_.id());
    scene_.destroyNow(component);
    if (owner != nullptr) selection_.select(owner->id());
}

std::shared_ptr<Widget> Details::body(Actor& owner) {
    const int id = owner.id();
    const std::string type(scene_.types().nameOf(owner));
    const Target self = [this, id]() -> void* { return targetOf(id, false); };

    auto column = make<VerticalBox>()
            + VerticalBox::slot().autoHeight()
                  [make<TextBox>()
                           .text([this, id] {
                               Actor* current = scene_.actor(id);
                               return current != nullptr ? current->name() : std::string();
                           })
                           .onTextCommitted([this, id](const std::string& text, TextCommit) {
                               Actor* current = scene_.actor(id);
                               if (current == nullptr || text.empty() || text == current->name()) return;
                               touched();
                               current->setName(text);
                           })]
            + VerticalBox::slot().autoHeight().padding(Margin(2.0f, 2.0f, 0.0f, 6.0f))
                  [make<Label>()
                           .text(type + "  #" + std::to_string(id))
                           .textStyle("Label.Small")
                           .colorAndOpacity(themeColor("Color.ForegroundDim"))];

    column + VerticalBox::slot().autoHeight().padding(Margin(0.0f, 0.0f, 0.0f, 4.0f))
                     [make<ExpandableArea>()
                              .areaTitle(type.empty() ? "Actor" : type)
                              .padding(Margin(6.0f, 6.0f))
                              .bodyContent(rows(owner.propList(), self))];
    column + VerticalBox::slot().autoHeight().padding(Margin(0.0f, 0.0f, 0.0f, 4.0f))
                     [make<ExpandableArea>()
                              .areaTitle("Components")
                              .padding(Margin(6.0f, 6.0f))
                              .bodyContent(componentsSection(owner))];

    if (auto* component = dynamic_cast<Component*>(editing())) {
        const int componentId = component->id();
        const std::string componentType(scene_.types().nameOf(*component));
        if (component->asSceneComponent() != nullptr) {
            const Target transform = [this, componentId]() -> void* { return targetOf(componentId, true); };
            column + VerticalBox::slot().autoHeight().padding(Margin(0.0f, 0.0f, 0.0f, 4.0f))
                             [make<ExpandableArea>()
                                      .areaTitle("Transform")
                                      .padding(Margin(6.0f, 6.0f))
                                      .bodyContent(rows(cinder::Reflect::props<cinder::Scene::Transform>(), transform))];
        }
        const Target props = [this, componentId]() -> void* { return targetOf(componentId, false); };
        const std::string title = component->name() == componentType ? componentType
                                                                     : component->name() + " (" + componentType + ")";
        column + VerticalBox::slot().autoHeight().padding(Margin(0.0f, 0.0f, 0.0f, 4.0f))
                         [make<ExpandableArea>()
                                  .areaTitle(title.empty() ? "Component" : title)
                                  .padding(Margin(6.0f, 6.0f))
                                  .bodyContent(rows(component->propList(), props))];
    }

    column + VerticalBox::slot().autoHeight()
                     [make<ExpandableArea>()
                              .areaTitle("Attributes")
                              .padding(Margin(6.0f, 6.0f))
                              .bodyContent(attributesSection(id))];
    return column;
}

}
