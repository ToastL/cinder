#include "Physics/World.hpp"

#include "Physics/RigidBodyComponent.hpp"
#include "Scene/Actor.hpp"
#include "Scene/Component.hpp"
#include "Scene/Scene.hpp"

namespace cinder::Physics {

using cinder::Scene::Actor;
using cinder::Scene::Component;

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
            Actor* actor = scene_.actor(id);
            if (actor == nullptr) continue;
            if (auto* body = actor->findComponent<RigidBodyComponent>()) body->wake();
        }
    }

    previous_.swap(current_);
    touched_.swap(touching_);
}

void World::signal(const Touch& touch, cinder::Scene::Signal signal) {
    const auto emitPair = [&](int self, int other, int selfCollider, int otherCollider) {
        Actor* actor = scene_.actor(self);
        Actor* against = scene_.actor(other);
        if (actor == nullptr || against == nullptr) return;
        Component* theirs = scene_.component(otherCollider);
        scene_.emit(*actor, signal, {against, theirs, {}});

        Component* mine = scene_.component(selfCollider);
        against = scene_.actor(other);
        theirs = scene_.component(otherCollider);
        if (mine != nullptr && against != nullptr) scene_.emit(*mine, signal, {against, theirs, {}});
    };

    emitPair(touch.a, touch.b, touch.colliderA, touch.colliderB);
    emitPair(touch.b, touch.a, touch.colliderB, touch.colliderA);
}

}
