#pragma once

#include <optional>

namespace cinder::scene {
class Node;
class Scene;
}

namespace cinder::dev {

class Selection {
public:
    std::optional<int> id() const { return id_; }
    bool selected(int id) const { return id_ == id; }

    void select(int id) { id_ = id; }
    void clear() { id_.reset(); }

    cinder::scene::Node* resolve(cinder::scene::Scene& scene);

private:
    std::optional<int> id_;
};

}
