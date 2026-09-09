#include "scene/Component.hpp"

#include "scene/Actor.hpp"

namespace cinder::scene {

Transform& Component::transform() const { return actor_->transform(); }

Scene& Component::scene() const { return actor_->scene(); }

}
