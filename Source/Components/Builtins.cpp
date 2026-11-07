#include "Components/Builtins.hpp"

#include "Components/Actors.hpp"
#include "Components/CameraComponent.hpp"
#include "Components/SpriteComponent.hpp"
#include "Components/StaticMeshComponent.hpp"
#include "Scene/Actor.hpp"
#include "Scene/ClassRegistry.hpp"
#include "Scene/SceneComponent.hpp"

namespace cinder::Components {

void registerBuiltins(cinder::Scene::ClassRegistry& types) {
    types.add<cinder::Scene::Actor>("Actor");
    types.add<StaticMeshActor>("StaticMeshActor");
    types.add<SpriteActor>("SpriteActor");
    types.add<CameraActor>("CameraActor");
    types.add<cinder::Scene::SceneComponent>("SceneComponent");
    types.add<StaticMeshComponent>("StaticMeshComponent");
    types.add<SpriteComponent>("SpriteComponent");
    types.add<CameraComponent>("CameraComponent");
}

}
