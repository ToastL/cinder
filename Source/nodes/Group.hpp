#pragma once

#include "scene/Spatial.hpp"

namespace cinder::nodes {

class Group final : public cinder::scene::Spatial {
public:
    CINDER_NODE(Group, cinder::scene::Spatial) {}
};

}
