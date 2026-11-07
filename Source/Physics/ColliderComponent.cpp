#include "Physics/ColliderComponent.hpp"

#include "Scene/Transform.hpp"

namespace cinder::Physics {

Geometry ColliderComponent::geometry() { return geometryOf(shape_, size_, transform().world()); }

}
