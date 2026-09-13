#include "dev/Selection.hpp"

#include "scene/Actor.hpp"
#include "scene/Scene.hpp"

namespace cinder::dev {

cinder::scene::Actor* Selection::resolve(cinder::scene::Scene& scene) {
    if (!id_) return nullptr;

    cinder::scene::Actor* actor = scene.byId(*id_);
    if (actor != nullptr && !actor->destroyed()) return actor;

    id_.reset();
    return nullptr;
}

}
