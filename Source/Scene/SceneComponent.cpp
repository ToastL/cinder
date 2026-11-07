#include "Scene/SceneComponent.hpp"

#include "Scene/Actor.hpp"

#include <algorithm>
#include <stdexcept>

namespace cinder::Scene {

bool SceneComponent::isAttachedTo(const SceneComponent* other) const {
    for (const SceneComponent* entry = parent_; entry != nullptr; entry = entry->parent_) {
        if (entry == other) return true;
    }
    return false;
}

void SceneComponent::attachTo(SceneComponent* parent) {
    if (parent == parent_) return;
    if (parent != nullptr) {
        if (parent == this || parent->isAttachedTo(this)) throw std::runtime_error("attaching would cycle the components");
        if (parent->owner() != owner()) throw std::runtime_error("a component attaches inside its own actor");
    }
    if (parent_ != nullptr) std::erase(parent_->children_, this);
    parent_ = parent;
    if (parent_ != nullptr) parent_->children_.push_back(this);
    transform_.dirty();
}

bool SceneComponent::isRoot() const { return owner() != nullptr && owner()->rootComponent() == this; }

Transform* SceneComponent::parentTransform() const {
    if (parent_ != nullptr) return &parent_->transform_;
    const Actor* actor = owner();
    if (actor == nullptr) return nullptr;

    SceneComponent* root = actor->rootComponent();
    if (root != nullptr && root != this) return &root->transform_;

    for (const Actor* above = actor->attachParent(); above != nullptr; above = above->attachParent()) {
        if (SceneComponent* parentRoot = above->rootComponent()) return &parentRoot->transform_;
    }
    return nullptr;
}

}
