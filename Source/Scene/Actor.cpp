#include "Scene/Actor.hpp"

#include "Scene/Attributes.hpp"
#include "Scene/Scene.hpp"
#include "Scene/SceneComponent.hpp"

#include <algorithm>
#include <stdexcept>

namespace cinder::Scene {

Actor::Actor() = default;

Actor::~Actor() = default;

SceneComponent* Actor::rootComponent() const {
    for (Component* component : components_) {
        SceneComponent* placed = component->asSceneComponent();
        if (placed == nullptr || placed->destroyed() || placed->attachParent() != nullptr || !placed->rootable()) continue;
        return placed;
    }
    return nullptr;
}

void Actor::setRootComponent(SceneComponent* component) {
    if (component == nullptr || component->owner() != this) return;
    component->attachTo(nullptr);
    auto found = std::find(components_.begin(), components_.end(), component);
    std::rotate(components_.begin(), found, found + 1);
    component->transform().dirty();
}

void Actor::arrange(const std::vector<Component*>& order) {
    std::vector<Component*> arranged;
    for (Component* component : order) {
        if (component->owner() == this && std::find(arranged.begin(), arranged.end(), component) == arranged.end()) {
            arranged.push_back(component);
        }
    }
    for (Component* component : components_) {
        if (std::find(arranged.begin(), arranged.end(), component) == arranged.end()) arranged.push_back(component);
    }
    components_ = std::move(arranged);
}

Transform* Actor::transform() const {
    SceneComponent* root = rootComponent();
    return root != nullptr ? &root->transform() : nullptr;
}

Component* Actor::findComponent(std::string_view name) const {
    for (Component* component : components_) {
        if (!component->destroyed() && component->name() == name) return component;
    }
    return nullptr;
}

Component* Actor::add(std::unique_ptr<Component> component, std::string name) {
    if (component == nullptr) return nullptr;
    if (scene() != nullptr) return scene()->addComponent(*this, std::move(component), std::move(name));

    Component* added = component.get();
    added->owner_ = this;
    if (!name.empty()) added->setName(std::move(name));
    owned_.push_back(std::move(component));
    components_.push_back(added);
    return added;
}

Actor* Actor::findAttached(std::string_view name) const {
    for (Actor* child : children_) {
        if (!child->destroyed() && child->name() == name) return child;
    }
    return nullptr;
}

bool Actor::isAncestorOf(const Actor* other) const {
    for (const Actor* entry = other; entry != nullptr; entry = entry->parent_) {
        if (entry == this) return true;
    }
    return false;
}

void Actor::attachTo(Actor* next) {
    if (next == this || isAncestorOf(next)) {
        throw std::runtime_error("attaching would cycle the actor tree");
    }
    if (scene() != nullptr) scene()->reattach(*this, next);
}

bool Actor::enabledInHierarchy() const {
    for (const Actor* entry = this; entry != nullptr; entry = entry->parent_) {
        if (!entry->isEnabled()) return false;
    }
    return true;
}

void Actor::destroy() {
    if (scene() != nullptr) scene()->destroy(this);
}

const PropValue* Actor::attribute(const std::string& name) const {
    auto found = attributes_.find(name);
    return found == attributes_.end() ? nullptr : &found->second;
}

void Actor::setAttribute(const std::string& name, PropValue value) {
    auto found = attributes_.find(name);
    if (found != attributes_.end() && sameAttribute(found->second, value)) return;

    attributes_.insert_or_assign(name, std::move(value));
    if (scene() != nullptr) scene()->attributeChanged(*this, name);
}

void Actor::removeAttribute(const std::string& name) {
    if (attributes_.erase(name) == 0) return;
    if (scene() != nullptr) scene()->attributeChanged(*this, name);
}

}
