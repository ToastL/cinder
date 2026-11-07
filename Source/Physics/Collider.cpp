#include "Physics/Collider.hpp"

#include "Scene/Transform.hpp"

namespace cinder::Physics {

Geometry Collider::geometry() { return geometryOf(shape_, size_, transform()->world()); }

}
