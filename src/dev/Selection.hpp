#pragma once

#include <optional>

namespace cinder::scene {
class Actor;
class Scene;
}

namespace cinder::dev {

class Selection {
public:
    std::optional<int> id() const { return id_; }
    bool selected(int id) const { return id_ == id; }

    void select(int id) { id_ = id; }
    void clear() { id_.reset(); }

    cinder::scene::Actor* resolve(cinder::scene::Scene& scene);

private:
    std::optional<int> id_;
};

}
