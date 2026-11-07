#include "Dev/Selection.hpp"

#include "Scene/Node.hpp"
#include "Scene/Scene.hpp"

namespace cinder::Dev {

cinder::Scene::Node* Selection::resolve(cinder::Scene::Scene& scene) {
    if (!id_) return nullptr;

    cinder::Scene::Node* node = scene.byId(*id_);
    if (node != nullptr && !node->destroyed()) return node;

    id_.reset();
    return nullptr;
}

}
