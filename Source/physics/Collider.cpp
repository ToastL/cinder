#include "physics/Collider.hpp"

#include "scene/Transform.hpp"

namespace cinder::physics {

Geometry Collider::geometry() { return geometryOf(shape_, size_, transform()->world()); }

}
