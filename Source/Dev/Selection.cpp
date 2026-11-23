#include "Dev/Selection.hpp"

#include "Scene/Actor.hpp"
#include "Scene/Component.hpp"
#include "Scene/Scene.hpp"
#include "Scene/SceneComponent.hpp"

#include <algorithm>
#include <utility>

namespace cinder::Dev {

bool Selection::isSelected(int id) const {
    if (id_ == id) return true;
    return std::find(extras_.begin(), extras_.end(), id) != extras_.end();
}

std::vector<int> Selection::ids() const {
    std::vector<int> out;
    if (!id_) return out;
    out.reserve(1 + extras_.size());
    out.push_back(*id_);
    for (const int extra : extras_) {
        if (extra != *id_) out.push_back(extra);
    }
    return out;
}

void Selection::toggle(int id) {
    pick_.cancel();
    reveal_ = false;
    if (id_ == id) {
        if (extras_.empty()) {
            id_.reset();
        } else {
            id_ = extras_.front();
            extras_.erase(extras_.begin());
        }
        return;
    }
    const auto found = std::find(extras_.begin(), extras_.end(), id);
    if (found != extras_.end()) {
        extras_.erase(found);
        return;
    }
    if (!id_) {
        id_ = id;
        return;
    }
    extras_.push_back(id);
}

void Selection::setMany(std::vector<int> ids, int primary, bool reveal) {
    pick_.cancel();
    id_ = primary;
    extras_.clear();
    extras_.reserve(ids.size());
    for (int id : ids) {
        if (id == primary) continue;
        if (std::find(extras_.begin(), extras_.end(), id) == extras_.end()) extras_.push_back(id);
    }
    reveal_ = reveal;
}

cinder::Scene::Object* Selection::resolve(cinder::Scene::Scene& scene) {
    extras_.erase(std::remove_if(extras_.begin(), extras_.end(),
                                 [&scene](int id) {
                                     const cinder::Scene::Object* object = scene.byId(id);
                                     return object == nullptr || object->destroyed();
                                 }),
                  extras_.end());
    if (!id_) return nullptr;

    cinder::Scene::Object* object = scene.byId(*id_);
    if (object != nullptr && !object->destroyed()) return object;

    id_.reset();
    if (!extras_.empty()) {
        id_ = extras_.front();
        extras_.erase(extras_.begin());
        return resolve(scene);
    }
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
