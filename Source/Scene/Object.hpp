#pragma once

#include "Reflect/Reflect.hpp"

#include <string>
#include <utility>

namespace cinder::Scene {

class Scene;

class Object {
public:
    Object() = default;
    virtual ~Object() = default;

    Object(const Object&) = delete;
    Object& operator=(const Object&) = delete;

    virtual const cinder::Reflect::PropList& propList() const = 0;
    virtual void* propTarget() = 0;
    virtual const void* propTarget() const = 0;

    int id() const { return id_; }
    Scene* scene() const { return scene_; }

    const std::string& name() const { return name_; }
    void setName(std::string name) { name_ = std::move(name); }

    bool isEnabled() const { return enabled_; }
    void setEnabled(bool value) { enabled_ = value; }

    int prefabId() const { return prefabId_; }
    void setPrefabId(int id) { prefabId_ = id; }

    bool started() const { return started_; }
    bool destroyed() const { return destroyed_; }

    CINDER_PROPS(Object, void) { CINDER_PROP(enabled_); }

private:
    friend class Scene;

    Scene* scene_ = nullptr;
    int id_ = 0;
    int prefabId_ = 0;
    std::string name_;
    bool enabled_ = true;
    bool started_ = false;
    bool destroyed_ = false;
};

}

#define CINDER_OBJECT(Type, Base)                                            \
public:                                                                  \
    const cinder::Reflect::PropList& propList() const override {             \
        return cinder::Reflect::props<Type>();                               \
    }                                                                    \
    void* propTarget() override { return this; }                         \
    const void* propTarget() const override { return this; }             \
    CINDER_PROPS(Type, Base)

#define CINDER_ACTOR(Type, Base) CINDER_OBJECT(Type, Base)
#define CINDER_COMPONENT(Type, Base) CINDER_OBJECT(Type, Base)
