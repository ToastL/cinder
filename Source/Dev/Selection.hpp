#pragma once

#include <optional>

namespace cinder::Scene {
class Node;
class Scene;
}

namespace cinder::Dev {

class Selection {
public:
    std::optional<int> id() const { return id_; }
    bool selected(int id) const { return id_ == id; }

    void select(int id, bool reveal = false) {
        id_ = id;
        reveal_ = reveal;
    }
    void clear() {
        id_.reset();
        reveal_ = false;
    }

    bool revealing() const { return reveal_; }
    void revealed() { reveal_ = false; }

    cinder::Scene::Node* resolve(cinder::Scene::Scene& scene);

private:
    std::optional<int> id_;
    bool reveal_ = false;
};

}
