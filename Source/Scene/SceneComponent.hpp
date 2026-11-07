#pragma once

#include "Scene/Component.hpp"
#include "Scene/Transform.hpp"

#include <vector>

namespace cinder::Scene {

class SceneComponent : public Component {
public:
    SceneComponent() : transform_(this) {}

    CINDER_COMPONENT(SceneComponent, Component) {}

    SceneComponent* asSceneComponent() override { return this; }
    const SceneComponent* asSceneComponent() const override { return this; }

    Transform& transform() { return transform_; }
    const Transform& transform() const { return transform_; }

    SceneComponent* attachParent() const { return parent_; }
    const std::vector<SceneComponent*>& attachChildren() const { return children_; }
    bool isAttachedTo(const SceneComponent* other) const;
    void attachTo(SceneComponent* parent);

    Transform* parentTransform() const;
    bool isRoot() const;

    virtual bool rootable() const { return true; }

private:
    friend class Actor;
    friend class Scene;

    Transform transform_;
    SceneComponent* parent_ = nullptr;
    std::vector<SceneComponent*> children_;
};

}
