#pragma once

#include "scene/Node.hpp"

namespace cinder::components {

class Folder final : public cinder::scene::Node {
public:
    CINDER_NODE(Folder, cinder::scene::Node) {}
};

}
