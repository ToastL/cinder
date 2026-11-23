#pragma once

#include "Dev/ReferencePick.hpp"

#include <optional>
#include <vector>

namespace cinder::Scene {
class Actor;
class Component;
class Object;
class Scene;
class SceneComponent;
}

namespace cinder::Dev {

class Selection {
public:
    std::optional<int> id() const { return id_; }
    bool selected(int id) const { return id_ == id; }
    bool isSelected(int id) const;
    std::vector<int> ids() const;
    const std::vector<int>& extras() const { return extras_; }
    std::size_t size() const { return id_ ? 1 + extras_.size() : 0; }

    void select(int id, bool reveal = false) {
        id_ = id;
        extras_.clear();
        reveal_ = reveal;
        pick_.cancel();
    }
    void clear() {
        id_.reset();
        extras_.clear();
        reveal_ = false;
        pick_.cancel();
    }
    void toggle(int id);
    void setMany(std::vector<int> ids, int primary, bool reveal = false);

    ReferencePick& pick() { return pick_; }
    const ReferencePick& pick() const { return pick_; }

    bool revealing() const { return reveal_; }
    void revealed() { reveal_ = false; }

    cinder::Scene::Object* resolve(cinder::Scene::Scene& scene);
    cinder::Scene::Actor* actor(cinder::Scene::Scene& scene);
    cinder::Scene::Component* component(cinder::Scene::Scene& scene);
    cinder::Scene::SceneComponent* placed(cinder::Scene::Scene& scene);

private:
    std::optional<int> id_;
    std::vector<int> extras_;
    bool reveal_ = false;
    ReferencePick pick_;
};

}
