#include "dev/Selection.hpp"

#include "scene/Node.hpp"
#include "scene/Scene.hpp"

namespace cinder::dev {

cinder::scene::Node* Selection::resolve(cinder::scene::Scene& scene) {
    if (!id_) return nullptr;

    cinder::scene::Node* node = scene.byId(*id_);
    if (node != nullptr && !node->destroyed()) return node;

    id_.reset();
    return nullptr;
}

}
