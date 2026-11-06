#include "physics/Nodes.hpp"

#include "physics/Body.hpp"
#include "physics/Collider.hpp"
#include "scene/NodeTypes.hpp"

namespace cinder::physics {

void registerNodes(cinder::scene::NodeTypes& types) {
    types.add<Body>("Body");
    types.add<Collider>("Collider");
}

}
