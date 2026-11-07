#include "Scene/Component.hpp"

#include "Scene/Actor.hpp"
#include "Scene/Scene.hpp"
#include "Scene/SceneComponent.hpp"

namespace cinder::Scene {

bool Component::activeInActor() const {
    if (!isEnabled()) return false;
    const SceneComponent* placed = asSceneComponent();
    if (placed == nullptr) return true;
    if (const SceneComponent* parent = placed->attachParent()) return parent->activeInActor();
    if (owner_ == nullptr) return true;
    const SceneComponent* root = owner_->rootComponent();
    return root == nullptr || root == placed || root->activeInActor();
}

bool Component::isDefault() const { return scene() != nullptr && scene()->isDefault(*this); }

void Component::destroy() {
    if (scene() != nullptr) scene()->destroy(this);
}

}
