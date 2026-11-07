#include "Physics/World.hpp"

#include "Physics/Body.hpp"
#include "Scene/Node.hpp"
#include "Scene/Scene.hpp"

namespace cinder::Physics {

using cinder::Scene::Node;

void World::notify() {
    current_.clear();
    for (const Touch& touch : touching_) current_.insert(touch.key);

    for (const Touch& touch : touching_) {
        if (previous_.find(touch.key) != previous_.end()) continue;
        signal(touch, cinder::Scene::Signal::Touched);
    }

    for (const Touch& touch : touched_) {
        if (current_.find(touch.key) != current_.end()) continue;
        signal(touch, cinder::Scene::Signal::TouchEnded);
    }

    for (const Touch& touch : touched_) {
        if (current_.find(touch.key) != current_.end()) continue;
        for (const int id : {touch.a, touch.b}) {
            if (auto* body = dynamic_cast<Body*>(scene_.byId(id))) body->wake();
        }
    }

    previous_.swap(current_);
    touched_.swap(touching_);
}

void World::signal(const Touch& touch, cinder::Scene::Signal signal) {
    Node* a = scene_.byId(touch.a);
    Node* b = scene_.byId(touch.b);
    if (a == nullptr || b == nullptr) return;
    scene_.emit(*a, signal, {b, {}});
    a = scene_.byId(touch.a);
    b = scene_.byId(touch.b);
    if (a != nullptr && b != nullptr) scene_.emit(*b, signal, {a, {}});
}

}
