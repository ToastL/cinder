#pragma once

#include "Scene/Spatial.hpp"

namespace cinder::Nodes {

class Group final : public cinder::Scene::Spatial {
public:
    CINDER_NODE(Group, cinder::Scene::Spatial) {}
};

}
