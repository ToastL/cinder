#pragma once

#include "Components/CameraComponent.hpp"
#include "Components/SpriteComponent.hpp"
#include "Components/StaticMeshComponent.hpp"
#include "Scene/Actor.hpp"

namespace cinder::Components {

class StaticMeshActor : public cinder::Scene::Actor {
public:
    StaticMeshActor() : mesh_(addComponent<StaticMeshComponent>()) {}

    StaticMeshComponent& mesh() const { return *mesh_; }

    CINDER_ACTOR(StaticMeshActor, cinder::Scene::Actor) {}

private:
    StaticMeshComponent* mesh_;
};

class SpriteActor : public cinder::Scene::Actor {
public:
    SpriteActor() : sprite_(addComponent<SpriteComponent>()) {}

    SpriteComponent& sprite() const { return *sprite_; }

    CINDER_ACTOR(SpriteActor, cinder::Scene::Actor) {}

private:
    SpriteComponent* sprite_;
};

class CameraActor : public cinder::Scene::Actor {
public:
    CameraActor() : camera_(addComponent<CameraComponent>()) {}

    CameraComponent& camera() const { return *camera_; }

    CINDER_ACTOR(CameraActor, cinder::Scene::Actor) {}

private:
    CameraComponent* camera_;
};

}
