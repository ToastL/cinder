#pragma once

#include "Scene/Actor.hpp"
#include "Scene/PropValue.hpp"
#include "Scene/SceneComponent.hpp"

#include <string>
#include <utility>

namespace cinder::Scene {

class ActorPlaceholder final : public Actor {
public:
    CINDER_ACTOR(ActorPlaceholder, Actor) {}

    ActorPlaceholder(std::string className, RawFields fields)
        : className_(std::move(className)), fields_(std::move(fields)) {}

    const std::string& className() const { return className_; }
    const RawFields& fields() const { return fields_; }

private:
    std::string className_;
    RawFields fields_;
};

class ComponentPlaceholder final : public SceneComponent {
public:
    CINDER_COMPONENT(ComponentPlaceholder, SceneComponent) {}

    ComponentPlaceholder(std::string className, RawFields fields)
        : className_(std::move(className)), fields_(std::move(fields)) {}

    const std::string& className() const { return className_; }
    const RawFields& fields() const { return fields_; }

    bool rootable() const override { return false; }

private:
    std::string className_;
    RawFields fields_;
};

}
