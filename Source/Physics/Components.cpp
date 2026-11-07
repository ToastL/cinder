#include "Physics/Components.hpp"

#include "Physics/RigidBodyComponent.hpp"
#include "Physics/ColliderComponent.hpp"
#include "Scene/ClassRegistry.hpp"

namespace cinder::Physics {

void registerComponents(cinder::Scene::ClassRegistry& types) {
    types.add<RigidBodyComponent>("RigidBodyComponent");
    types.add<ColliderComponent>("ColliderComponent");
}

}
