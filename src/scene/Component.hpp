#pragma once

#include "reflect/Reflect.hpp"

namespace cinder::scene {

class Actor;
class DrawList;
class Scene;
class Transform;

class Component {
public:
    virtual ~Component() = default;

    virtual const cinder::reflect::PropList& propList() const = 0;
    virtual void* propTarget() = 0;
    virtual const void* propTarget() const = 0;

    Actor* actor() const { return actor_; }
    Transform& transform() const;
    Scene& scene() const;

    bool isEnabled() const { return enabled_; }
    void setEnabled(bool value) { enabled_ = value; }
    bool started() const { return started_; }

    virtual void onStart() {}
    virtual void onUpdate(float dt) {}
    virtual void onRender(float alpha, DrawList& draws) {}
    virtual void onDestroy() {}

    CINDER_PROPS(Component, void) { CINDER_PROP(enabled_); }

private:
    friend class Actor;
    friend class Scene;

    Actor* actor_ = nullptr;
    bool started_ = false;
    bool enabled_ = true;
};

}

#define CINDER_COMPONENT(Type, Base)                                        \
public:                                                                 \
    const cinder::reflect::PropList& propList() const override {            \
        return cinder::reflect::props<Type>();                              \
    }                                                                   \
    void* propTarget() override { return this; }                        \
    const void* propTarget() const override { return this; }            \
    CINDER_PROPS(Type, Base)
