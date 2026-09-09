#include "scene/Actor.hpp"

#include "scene/Scene.hpp"

#include <algorithm>
#include <stdexcept>

namespace cinder::scene {

Actor::Actor(Scene& scene, int id, std::string name)
    : scene_(scene), id_(id), transform_(this), name_(std::move(name)) {}

bool Actor::active() const {
    for (const Actor* entry = this; entry != nullptr; entry = entry->parent_) {
        if (!entry->active_) return false;
    }
    return true;
}

Component* Actor::add(std::unique_ptr<Component> component) {
    if (component == nullptr) return nullptr;
    if (component->actor_ != nullptr) {
        throw std::runtime_error("component is already attached to actor "
                                 + std::to_string(component->actor_->id()));
    }

    component->actor_ = this;
    Component* raw = component.get();
    components_.push_back(std::move(component));
    scene_.queueStart(raw);
    return raw;
}

Component* Actor::get(std::type_index type) const {
    for (const std::unique_ptr<Component>& component : components_) {
        Component* raw = component.get();
        if (std::type_index(typeid(*raw)) == type) return raw;
    }
    return nullptr;
}

void Actor::remove(Component* component) {
    auto found = std::find_if(components_.begin(), components_.end(),
                              [component](const std::unique_ptr<Component>& held) {
                                  return held.get() == component;
                              });
    if (found == components_.end()) return;

    scene_.unqueueStart(component);
    if (component->started_) component->onDestroy();
    component->actor_ = nullptr;
    components_.erase(found);
}

void Actor::setParent(Actor* next) {
    if (next == this || isAncestorOf(next)) {
        throw std::runtime_error("reparenting would cycle the actor tree");
    }

    if (parent_ != nullptr) {
        std::vector<Actor*>& siblings = parent_->children_;
        siblings.erase(std::remove(siblings.begin(), siblings.end(), this), siblings.end());
    } else {
        scene_.detachRoot(this);
    }

    parent_ = next;
    if (next != nullptr) next->children_.push_back(this);
    else scene_.attachRoot(this);

    transform_.dirty();
}

Actor* Actor::spawnChild(const std::string& name) { return scene_.spawn(name, this); }

void Actor::destroy() { scene_.destroy(this); }

bool Actor::isAncestorOf(const Actor* other) const {
    for (const Actor* entry = other; entry != nullptr; entry = entry->parent_) {
        if (entry == this) return true;
    }
    return false;
}

void Actor::markDestroyed() {
    destroyed_ = true;
    for (Actor* child : children_) child->markDestroyed();
}

}
