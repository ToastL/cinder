#pragma once

#include "scene/Node.hpp"

namespace cinder::nodes {

class Folder final : public cinder::scene::Node {
public:
    CINDER_NODE(Folder, cinder::scene::Node) {}
};

}
