#include "Scene/Scene.hpp"

#include "Scene/Actor.hpp"
#include "Scene/ClassRegistry.hpp"
#include "Scene/Component.hpp"
#include "Scene/DrawList.hpp"
#include "Scene/Placeholder.hpp"
#include "Scene/Ref.hpp"
#include "Scene/SceneComponent.hpp"
#include "Scene/Transform.hpp"

#include <algorithm>
#include <stdexcept>
#include <typeinfo>

namespace cinder::Scene {
namespace {

Actor* findIn(Actor& actor, std::string_view name) {
    if (!actor.destroyed() && actor.name() == name) return &actor;
    for (Actor* child : actor.attachedActors()) {
        if (Actor* found = findIn(*child, name)) return found;
    }
    return nullptr;
}

std::unique_ptr<Component> blankOf(const ClassRegistry& types, const Component& source) {
    if (const auto* placeholder = dynamic_cast<const ComponentPlaceholder*>(&source)) {
        return std::make_unique<ComponentPlaceholder>(placeholder->className(), placeholder->fields());
    }
    return types.createComponent(types.nameOf(source));
}

void copyComponent(const Component& source, Component& copy) {
    cinder::Reflect::copyProps(source.propList(), source.propTarget(), copy.propTarget());
    const SceneComponent* from = source.asSceneComponent();
    SceneComponent* to = copy.asSceneComponent();
    if (from != nullptr && to != nullptr) {
        cinder::Reflect::copyProps(cinder::Reflect::props<Transform>(), &from->transform(), &to->transform());
    }
}

}

Scene::Scene(ClassRegistry& types) : types_(types) {}

Scene::~Scene() { clear(); }

void Scene::enroll(Object& object, std::optional<int> id) {
    if (id && (*id < 1 || *id > MAX_ID)) throw std::runtime_error("id " + std::to_string(*id) + " is out of range");
    if (!id && nextId_ > MAX_ID) throw std::runtime_error("the scene has run out of ids");
    const int assigned = id.value_or(nextId_);
    if (objects_.count(assigned) != 0) throw std::runtime_error("id " + std::to_string(assigned) + " is taken");

    object.scene_ = this;
    object.id_ = assigned;
    if (object.name_.empty()) {
        const std::string_view type = types_.nameOf(object);
        object.name_ = type.empty() ? std::string(dynamic_cast<Actor*>(&object) != nullptr ? "Actor" : "Component")
                                    : std::string(type);
    }
    objects_.emplace(assigned, &object);
    if (assigned >= nextId_) nextId_ = assigned + 1;
}

Actor* Scene::insert(std::unique_ptr<Actor> owned, Actor* parent, std::optional<int> id,
                     const std::vector<IdHint>& componentIds) {
    if (owned == nullptr) return nullptr;
    if (parent != nullptr && parent->scene_ != this) {
        throw std::runtime_error("a parent must belong to the same scene");
    }
    if (owned->scene_ != nullptr) throw std::runtime_error("an actor can be inserted once");

    Actor* actor = owned.get();
    enroll(*actor, id);
    actors_.emplace(actor->id_, std::move(owned));

    for (const IdHint& hint : componentIds) {
        for (Component* component : actor->components_) {
            if (component == hint.component && component->scene_ == nullptr) enroll(*component, hint.id);
        }
    }
    for (Component* component : actor->components_) {
        if (component->scene_ == nullptr) enroll(*component, std::nullopt);
    }

    attach(*actor, parent);
    pendingStart_.push_back(actor);
    return actor;
}

Actor* Scene::spawn(std::string_view className, Actor* parent) {
    std::unique_ptr<Actor> actor = types_.createActor(className);
    return actor == nullptr ? nullptr : insert(std::move(actor), parent);
}

Component* Scene::addComponent(Actor& actor, std::unique_ptr<Component> owned, std::string name,
                               SceneComponent* attachTo) {
    if (owned == nullptr) return nullptr;
    if (actor.scene_ != this) throw std::runtime_error("the actor must belong to this scene");
    if (attachTo != nullptr && attachTo->owner_ != &actor) {
        throw std::runtime_error("a component attaches inside its own actor");
    }

    Component* component = owned.get();
    component->owner_ = &actor;
    if (!name.empty()) component->setName(std::move(name));
    actor.owned_.push_back(std::move(owned));
    actor.components_.push_back(component);
    enroll(*component, std::nullopt);

    if (SceneComponent* placed = component->asSceneComponent()) {
        if (attachTo != nullptr) placed->attachTo(attachTo);
        placed->transform().dirty();
    }

    if (actor.started_) pendingStart_.push_back(component);
    emit(actor, Signal::ComponentAdded, {nullptr, component, {}});
    return component;
}

Component* Scene::addComponent(Actor& actor, std::string_view className, SceneComponent* attachTo) {
    std::unique_ptr<Component> component = types_.createComponent(className);
    return component == nullptr ? nullptr : addComponent(actor, std::move(component), {}, attachTo);
}

Actor* Scene::clone(const Actor& source, Actor* parent) {
    std::unordered_map<int, Object*> copies;
    Actor* actor = cloneTree(source, parent, copies);
    for (const auto& [from, copy] : copies) {
        forEachRef(*copy, [&](const cinder::Reflect::PropDef& def, void* target) {
            auto found = copies.find(def.readInt(target));
            if (found != copies.end()) def.writeInt(target, found->second->id());
        });
    }
    return actor;
}

Actor* Scene::cloneTree(const Actor& source, Actor* parent, std::unordered_map<int, Object*>& copies) {
    std::unique_ptr<Actor> copy;
    if (const auto* placeholder = dynamic_cast<const ActorPlaceholder*>(&source)) {
        copy = std::make_unique<ActorPlaceholder>(placeholder->className(), placeholder->fields());
    } else {
        copy = types_.createActor(types_.nameOf(source));
    }
    if (copy == nullptr) return nullptr;

    cinder::Reflect::copyProps(source.propList(), source.propTarget(), copy->propTarget());
    copy->setName(source.name());
    copy->attributes_ = source.attributes_;

    std::unordered_map<const Component*, Component*> matched;
    std::vector<Component*> unclaimed = copy->components_;
    for (const Component* from : source.components_) {
        if (from->destroyed()) continue;
        Component* to = nullptr;
        if (isDefault(*from)) {
            auto found = std::find_if(unclaimed.begin(), unclaimed.end(), [&](Component* candidate) {
                return types_.displayName(*candidate) == from->name() && typeid(*candidate) == typeid(*from);
            });
            if (found != unclaimed.end()) {
                to = *found;
                unclaimed.erase(found);
            }
        }
        if (to == nullptr) {
            std::unique_ptr<Component> blank = blankOf(types_, *from);
            if (blank == nullptr) continue;
            to = copy->add(std::move(blank), from->name());
        }
        copyComponent(*from, *to);
        to->setName(from->name());
        matched.emplace(from, to);
    }

    for (const auto& [from, to] : matched) {
        const SceneComponent* placed = from->asSceneComponent();
        SceneComponent* target = to->asSceneComponent();
        if (placed == nullptr || target == nullptr) continue;
        auto parentCopy = placed->attachParent() != nullptr ? matched.find(placed->attachParent()) : matched.end();
        target->attachTo(parentCopy != matched.end() ? parentCopy->second->asSceneComponent() : nullptr);
    }
    if (const SceneComponent* root = source.rootComponent()) {
        auto found = matched.find(root);
        if (found != matched.end()) copy->setRootComponent(found->second->asSceneComponent());
    }

    const std::vector<Actor*> children = source.children_;
    Actor* actor = insert(std::move(copy), parent);
    copies.emplace(source.id(), actor);
    for (const auto& [from, to] : matched) copies.emplace(from->id(), to);
    for (Actor* child : children) {
        if (!child->destroyed_) cloneTree(*child, actor, copies);
    }
    return actor;
}

Object* Scene::byId(int id) const {
    auto found = objects_.find(id);
    return found == objects_.end() ? nullptr : found->second;
}

Actor* Scene::actor(int id) const {
    auto found = actors_.find(id);
    return found == actors_.end() ? nullptr : found->second.get();
}

Component* Scene::component(int id) const { return dynamic_cast<Component*>(byId(id)); }

Actor* Scene::find(std::string_view name) const {
    for (Actor* root : roots_) {
        if (Actor* found = findIn(*root, name)) return found;
    }
    return nullptr;
}

const Component* Scene::archetype(const Component& component) const {
    const Actor* owner = component.owner_;
    if (owner == nullptr) return nullptr;
    const auto* base = dynamic_cast<const Actor*>(types_.fallback(types_.nameOf(*owner)));
    if (base == nullptr) return nullptr;
    for (const Component* candidate : base->components_) {
        if (types_.displayName(*candidate) == types_.displayName(component) && typeid(*candidate) == typeid(component)) {
            return candidate;
        }
    }
    return nullptr;
}

bool Scene::isDefault(const Component& component) const { return archetype(component) != nullptr; }

void Scene::update(float dt) {
    startPending();
    runTimers(dt);
    for (std::size_t i = 0; i < roots_.size(); ++i) update(*roots_[i], dt);
    flushDestroy();
}

void Scene::update(Actor& actor, float dt) {
    if (actor.destroyed_ || !actor.enabled_) return;
    if (actor.started_) actor.tick(dt);
    for (std::size_t i = 0; i < actor.components_.size(); ++i) {
        Component* component = actor.components_[i];
        if (component->destroyed_ || !component->started_ || !component->activeInActor()) continue;
        component->tick(dt);
    }
    for (std::size_t i = 0; i < actor.children_.size(); ++i) update(*actor.children_[i], dt);
}

void Scene::render(float alpha, DrawList& draws) {
    for (std::size_t i = 0; i < roots_.size(); ++i) render(*roots_[i], alpha, draws);
}

void Scene::render(Actor& actor, float alpha, DrawList& draws) {
    if (actor.destroyed_ || !actor.enabled_) return;
    for (std::size_t i = 0; i < actor.components_.size(); ++i) {
        Component* component = actor.components_[i];
        if (component->destroyed_ || !component->activeInActor()) continue;
        component->onRender(alpha, draws);
    }
    for (std::size_t i = 0; i < actor.children_.size(); ++i) render(*actor.children_[i], alpha, draws);
}

void Scene::start(Object& object) {
    object.started_ = true;
    if (auto* actor = dynamic_cast<Actor*>(&object)) actor->beginPlay();
    else static_cast<Component&>(object).beginPlay();
}

void Scene::startPending() {
    for (std::size_t i = 0; i < pendingStart_.size(); ++i) {
        Object* object = pendingStart_[i];
        if (object == nullptr || object->destroyed_ || object->started_) continue;

        if (auto* actor = dynamic_cast<Actor*>(object)) {
            for (std::size_t c = 0; c < actor->components_.size(); ++c) {
                Component* component = actor->components_[c];
                if (!component->started_ && !component->destroyed_) start(*component);
                if (actor->destroyed_) break;
            }
            if (!actor->destroyed_) start(*actor);
            continue;
        }

        auto* component = static_cast<Component*>(object);
        if (component->owner_ != nullptr && component->owner_->started_) start(*component);
    }
    pendingStart_.clear();
}

void Scene::destroy(Actor* actor) {
    if (actor == nullptr || actor->destroyed_) return;
    markDestroyed(*actor);
    pendingDestroy_.push_back(actor->id_);
}

void Scene::destroy(Component* component) {
    if (component == nullptr || component->destroyed_) return;
    markDestroyed(*component);
    pendingDestroy_.push_back(component->id_);
}

void Scene::destroyNow(Actor* actor) {
    if (actor == nullptr || this->actor(actor->id_) != actor) return;
    markDestroyed(*actor);
    detach(*actor);
    teardown(*actor, false);
}

void Scene::destroyNow(Component* component) {
    if (component == nullptr || byId(component->id_) != component) return;
    markDestroyed(*component);
    remove(*component);
}

void Scene::flushDestroy() {
    for (std::size_t i = 0; i < pendingDestroy_.size(); ++i) {
        Object* object = byId(pendingDestroy_[i]);
        if (object == nullptr) continue;
        if (auto* actor = dynamic_cast<Actor*>(object)) {
            detach(*actor);
            teardown(*actor, false);
        } else {
            remove(static_cast<Component&>(*object));
        }
    }
    pendingDestroy_.clear();
}

void Scene::markDestroyed(Actor& actor) {
    actor.destroyed_ = true;
    for (Component* component : actor.components_) component->destroyed_ = true;
    for (Actor* child : actor.children_) markDestroyed(*child);
}

void Scene::markDestroyed(Component& component) {
    component.destroyed_ = true;
    if (SceneComponent* placed = component.asSceneComponent()) {
        for (SceneComponent* child : placed->children_) markDestroyed(*child);
    }
}

void Scene::remove(Component& component) {
    if (SceneComponent* placed = component.asSceneComponent()) {
        const std::vector<SceneComponent*> children = placed->children_;
        for (std::size_t i = children.size(); i-- > 0;) remove(*children[i]);
        placed->attachTo(nullptr);
    }

    unqueueStart(&component);
    if (component.started_) component.endPlay();
    dropHandlers(component.id_);
    objects_.erase(component.id_);

    Actor& owner = *component.owner_;
    std::erase(owner.components_, &component);
    std::erase_if(owner.owned_, [&](const std::unique_ptr<Component>& held) { return held.get() == &component; });
}

void Scene::teardown(Actor& actor, bool clearing) {
    if (!clearing) emit(actor, Signal::Destroying);

    const std::vector<Actor*> children = actor.children_;
    for (std::size_t i = children.size(); i-- > 0;) teardown(*children[i], clearing);
    actor.children_.clear();

    if (!clearing) unqueueStart(&actor);
    for (std::size_t i = actor.components_.size(); i-- > 0;) {
        Component* component = actor.components_[i];
        if (!clearing) unqueueStart(component);
        if (component->started_) component->endPlay();
        if (!clearing) dropHandlers(component->id_);
        objects_.erase(component->id_);
    }
    if (actor.started_) actor.endPlay();
    if (!clearing) dropHandlers(actor.id_);
    objects_.erase(actor.id_);
    actors_.erase(actor.id_);
}

void Scene::clear() {
    const std::vector<Actor*> roots = roots_;
    for (std::size_t i = roots.size(); i-- > 0;) teardown(*roots[i], true);
    roots_.clear();
    actors_.clear();
    objects_.clear();
    pendingStart_.clear();
    pendingDestroy_.clear();
    connections_.clear();
    timers_.clear();
    time_ = 0.0;
}

void Scene::reattach(Actor& actor, Actor* next) {
    if (next != nullptr && next->scene_ != this) {
        throw std::runtime_error("a parent must belong to the same scene");
    }
    detach(actor);
    attach(actor, next);
}

void Scene::attach(Actor& actor, Actor* parent) {
    actor.parent_ = parent;
    if (parent == nullptr) roots_.push_back(&actor);
    else parent->children_.push_back(&actor);

    if (Transform* transform = actor.transform()) transform->dirty();
    if (parent != nullptr) emit(*parent, Signal::ChildAdded, {&actor, nullptr, {}});
}

void Scene::detach(Actor& actor) {
    Actor* parent = actor.parent_;
    std::vector<Actor*>& siblings = parent == nullptr ? roots_ : parent->children_;
    siblings.erase(std::remove(siblings.begin(), siblings.end(), &actor), siblings.end());
    actor.parent_ = nullptr;

    if (parent != nullptr) emit(*parent, Signal::ChildRemoved, {&actor, nullptr, {}});
}

void Scene::unqueueStart(const Object* object) {
    for (Object*& pending : pendingStart_) {
        if (pending == object) pending = nullptr;
    }
}

void Scene::attributeChanged(Actor& actor, const std::string& name) {
    emit(actor, Signal::AttributeChanged, {nullptr, nullptr, name});
}

void Scene::hook(Object& object, Signal signal, const SignalArgs& args) {
    if (auto* actor = dynamic_cast<Actor*>(&object)) {
        switch (signal) {
            case Signal::AttributeChanged: actor->onAttributeChanged(args.name); break;
            case Signal::ChildAdded: actor->onChildAdded(*args.other); break;
            case Signal::ChildRemoved: actor->onChildRemoved(*args.other); break;
            case Signal::ComponentAdded: actor->onComponentAdded(*args.component); break;
            case Signal::Touched: actor->onTouched(*args.other); break;
            case Signal::TouchEnded: actor->onTouchEnded(*args.other); break;
            case Signal::Destroying: break;
        }
        return;
    }

    auto& component = static_cast<Component&>(object);
    switch (signal) {
        case Signal::Touched: component.onTouched(*args.other); break;
        case Signal::TouchEnded: component.onTouchEnded(*args.other); break;
        default: break;
    }
}

void Scene::emit(Object& source, Signal signal, const SignalArgs& args) {
    if (source.started_ && !source.destroyed_) hook(source, signal, args);

    std::vector<int> handles;
    for (const Connection& connection : connections_) {
        if (connection.source == source.id_ && connection.signal == signal) handles.push_back(connection.id);
    }
    for (const int handle : handles) {
        auto found = std::find_if(connections_.begin(), connections_.end(),
                                  [handle](const Connection& connection) { return connection.id == handle; });
        if (found == connections_.end()) continue;
        const Object* owner = byId(found->owner);
        if (owner == nullptr || !owner->started_ || owner->destroyed_) continue;
        const SignalHandler handler = found->handler;
        handler(args);
    }
}

int Scene::connect(Object& source, Signal signal, Object& owner, SignalHandler handler) {
    const int handle = nextHandle_++;
    connections_.push_back(Connection{handle, source.id_, signal, owner.id_, std::move(handler)});
    return handle;
}

void Scene::disconnect(int connection) {
    std::erase_if(connections_, [connection](const Connection& entry) { return entry.id == connection; });
}

int Scene::after(Object& owner, double seconds, TimerHandler handler) {
    return schedule(owner, seconds, false, std::move(handler));
}

int Scene::every(Object& owner, double seconds, TimerHandler handler) {
    return schedule(owner, seconds, true, std::move(handler));
}

int Scene::schedule(Object& owner, double seconds, bool repeat, TimerHandler handler) {
    const double interval = std::max(seconds, 0.0);
    const int handle = nextHandle_++;
    timers_.push_back(Timer{handle, owner.id_, time_ + interval, interval, repeat, std::move(handler)});
    return handle;
}

void Scene::cancel(int timer) {
    std::erase_if(timers_, [timer](const Timer& entry) { return entry.id == timer; });
}

void Scene::runTimers(double dt) {
    time_ += dt;

    std::vector<int> due;
    for (const Timer& timer : timers_) {
        if (timer.due <= time_) due.push_back(timer.id);
    }
    for (const int handle : due) {
        auto found = std::find_if(timers_.begin(), timers_.end(),
                                  [handle](const Timer& timer) { return timer.id == handle; });
        if (found == timers_.end()) continue;
        const Object* owner = byId(found->owner);
        if (owner == nullptr || owner->destroyed_) {
            timers_.erase(found);
            continue;
        }

        const TimerHandler handler = found->handler;
        if (found->repeat) {
            found->due = std::max(found->due + found->interval, time_);
        } else {
            timers_.erase(found);
        }
        handler();
    }
}

void Scene::dropHandlers(int object) {
    std::erase_if(connections_, [object](const Connection& entry) { return entry.source == object || entry.owner == object; });
    std::erase_if(timers_, [object](const Timer& entry) { return entry.owner == object; });
}

}
