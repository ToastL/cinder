#include "Physics/Nodes.hpp"

#include "Physics/Body.hpp"
#include "Physics/Collider.hpp"
#include "Scene/NodeTypes.hpp"

namespace cinder::Physics {

void registerNodes(cinder::Scene::NodeTypes& types) {
    types.add<Body>("Body");
    types.add<Collider>("Collider");
}

}
