#pragma once

#include "scene/Node.hpp"
#include "scene/PropValue.hpp"

#include <string>
#include <utility>

namespace cinder::scene {

class Placeholder final : public Node {
public:
    CINDER_NODE(Placeholder, Node) {}

    Placeholder(std::string className, RawFields fields)
        : className_(std::move(className)), fields_(std::move(fields)) {}

    const std::string& className() const { return className_; }
    const RawFields& fields() const { return fields_; }

private:
    std::string className_;
    RawFields fields_;
};

}
