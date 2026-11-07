#include "Dev/Selection.hpp"

#include "Scene/Actor.hpp"
#include "Scene/Component.hpp"
#include "Scene/Scene.hpp"
#include "Scene/SceneComponent.hpp"

namespace cinder::Dev {

cinder::Scene::Object* Selection::resolve(cinder::Scene::Scene& scene) {
    if (!id_) return nullptr;

    cinder::Scene::Object* object = scene.byId(*id_);
    if (object != nullptr && !object->destroyed()) return object;

    id_.reset();
    return nullptr;
}

cinder::Scene::Actor* Selection::actor(cinder::Scene::Scene& scene) {
    cinder::Scene::Object* object = resolve(scene);
    if (auto* actor = dynamic_cast<cinder::Scene::Actor*>(object)) return actor;
    if (auto* component = dynamic_cast<cinder::Scene::Component*>(object)) return component->owner();
    return nullptr;
}

cinder::Scene::Component* Selection::component(cinder::Scene::Scene& scene) {
    return dynamic_cast<cinder::Scene::Component*>(resolve(scene));
}

cinder::Scene::SceneComponent* Selection::placed(cinder::Scene::Scene& scene) {
    cinder::Scene::Object* object = resolve(scene);
    if (auto* component = dynamic_cast<cinder::Scene::Component*>(object)) return component->asSceneComponent();
    if (auto* actor = dynamic_cast<cinder::Scene::Actor*>(object)) return actor->rootComponent();
    return nullptr;
}

}
