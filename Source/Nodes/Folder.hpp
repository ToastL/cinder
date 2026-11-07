#pragma once

#include "Scene/Node.hpp"

namespace cinder::Nodes {

class Folder final : public cinder::Scene::Node {
public:
    CINDER_NODE(Folder, cinder::Scene::Node) {}
};

}
