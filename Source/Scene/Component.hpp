#pragma once

#include "Scene/Object.hpp"

namespace cinder::Scene {

class Actor;
class DrawList;
class SceneComponent;

class Component : public Object {
public:
    Actor* owner() const { return owner_; }
    bool isDefault() const;

    virtual SceneComponent* asSceneComponent() { return nullptr; }
    virtual const SceneComponent* asSceneComponent() const { return nullptr; }

    bool activeInActor() const;
    void destroy();

    virtual void beginPlay() {}
    virtual void tick(float dt) {}
    virtual void onRender(float alpha, DrawList& draws) {}
    virtual void endPlay() {}
    virtual void onTouched(Actor& other) {}
    virtual void onTouchEnded(Actor& other) {}

    CINDER_PROPS(Component, Object) {}

private:
    friend class Actor;
    friend class Scene;

    Actor* owner_ = nullptr;
};

}
