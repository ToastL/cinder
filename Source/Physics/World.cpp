#include "Physics/World.hpp"

#include "Physics/RigidBodyComponent.hpp"
#include "Physics/Collide.hpp"
#include "Physics/ColliderComponent.hpp"
#include "Physics/Pose.hpp"
#include "Scene/Actor.hpp"
#include "Scene/Scene.hpp"
#include "Scene/Transform.hpp"

#include <glm/geometric.hpp>
#include <glm/matrix.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace cinder::Physics {

using cinder::Scene::Actor;
using cinder::Scene::Transform;

namespace {

ContactKey contactKey(int a, int b, int feature) { return ContactKey{a, b, feature}; }

bool finite(const glm::vec3& v) { return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z); }

}

void World::step(float dt) {
    if (dt <= 0.0f) return;

    states_.clear();
    proxies_.clear();
    touching_.clear();
    states_.emplace_back();

    for (Actor* root : scene_.roots()) gather(*root, 0);
    for (std::size_t i = 1; i < states_.size(); ++i) place(states_[i], dt);
    for (Proxy& proxy : proxies_) {
        proxy.geometry = proxy.collider->geometry();
        proxy.bounds = boundsOf(proxy.geometry);
    }
    std::erase_if(proxies_, [](const Proxy& proxy) {
        return !finite(proxy.bounds.min) || !finite(proxy.bounds.max);
    });
    weigh();
    for (std::size_t i = 1; i < states_.size(); ++i) accelerate(states_[i], dt);
    solver_.begin(states_);
    detect(dt);
    solver_.warmStart();
    solver_.solve();
    rest(dt);
    for (std::size_t i = 1; i < states_.size(); ++i) advance(states_[i], dt);
    notify();
}

void World::gather(Actor& actor, int owner) {
    if (actor.destroyed() || !actor.isEnabled()) return;

    auto* body = actor.findComponent<RigidBodyComponent>();
    if (body != nullptr && body->activeInActor() && body->transform() != nullptr) {
        BodyState state;
        state.body = body;
        states_.push_back(state);
        owner = static_cast<int>(states_.size()) - 1;
    }

    for (cinder::Scene::Component* component : actor.components()) {
        auto* collider = dynamic_cast<ColliderComponent*>(component);
        if (collider == nullptr || collider->destroyed() || !collider->activeInActor()) continue;
        Proxy proxy;
        proxy.collider = collider;
        proxy.owner = owner;
        proxies_.push_back(proxy);
    }

    for (Actor* child : actor.attachedActors()) gather(*child, owner);
}

void World::place(BodyState& state, float dt) {
    RigidBodyComponent& body = *state.body;
    if (body.velocity_ != body.wroteVelocity_ || body.angularVelocity_ != body.wroteSpin_) body.wake();

    const glm::mat4& world = body.transform()->world();
    if (body.motion_ == RigidBodyComponent::Motion::Dynamic && body.simulated_ && world == body.placed_) return;

    body.wake();

    const glm::quat previous = body.orientation_;
    body.origin_ = glm::vec3(world[3]);
    body.orientation_ = orientationOf(world);
    if (body.motion_ == RigidBodyComponent::Motion::Kinematic) {
        body.angularVelocity_ =
            body.simulated_ ? angularVelocityOf(previous, body.orientation_, dt) : glm::vec3(0.0f);
    }
}

void World::weigh() {
    for (const Proxy& proxy : proxies_) {
        if (proxy.owner == 0) continue;
        BodyState& state = states_[static_cast<std::size_t>(proxy.owner)];
        const float volume = volumeOf(proxy.geometry);
        state.volume += volume;
        state.moment += proxy.geometry.center * volume;
    }

    for (std::size_t i = 1; i < states_.size(); ++i) {
        BodyState& state = states_[i];
        state.center = state.volume > 0.0f ? state.moment / state.volume : state.body->origin_;
    }

    for (const Proxy& proxy : proxies_) {
        if (proxy.owner == 0) continue;
        BodyState& state = states_[static_cast<std::size_t>(proxy.owner)];
        if (state.volume <= 0.0f) continue;

        const float mass = state.body->mass_ * volumeOf(proxy.geometry) / state.volume;
        const glm::vec3 offset = proxy.geometry.center - state.center;
        state.inertia += inertiaOf(proxy.geometry, mass) +
                         mass * (glm::dot(offset, offset) * glm::mat3(1.0f) - glm::outerProduct(offset, offset));
    }

    for (std::size_t i = 1; i < states_.size(); ++i) {
        BodyState& state = states_[i];
        if (state.body->motion_ != RigidBodyComponent::Motion::Dynamic) continue;
        if (state.body->asleep_) continue;
        state.inverseMass = 1.0f / state.body->mass_;
        if (glm::determinant(state.inertia) > 0.0f) state.inverseInertia = glm::inverse(state.inertia);
        if (!state.body->planar_) continue;

        state.linearMask = glm::vec3(1.0f, 1.0f, 0.0f);
        const glm::mat3 spin(glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        state.inverseInertia = spin * state.inverseInertia * spin;
    }
}

void World::accelerate(BodyState& state, float dt) {
    RigidBodyComponent& body = *state.body;
    switch (body.motion_) {
        case RigidBodyComponent::Motion::Static:
            return;
        case RigidBodyComponent::Motion::Kinematic:
            body.velocity_ = body.simulated_ ? (state.center - body.center_) / dt : glm::vec3(0.0f);
            state.velocity = body.velocity_;
            state.angularVelocity = body.angularVelocity_;
            return;
        case RigidBodyComponent::Motion::Dynamic:
            if (body.asleep_) return;
            state.velocity = (body.velocity_ + gravity_ * (body.gravityScale_ * dt) +
                              (body.force_ * dt + body.impulse_) * state.inverseMass) /
                             (1.0f + dt * body.linearDamping_);
            state.angularVelocity =
                (body.angularVelocity_ +
                 state.inverseInertia * (body.torque_ * dt + body.angularImpulse_)) /
                (1.0f + dt * body.angularDamping_);
            state.velocity *= state.linearMask;
            if (body.planar_) state.angularVelocity = glm::vec3(0.0f, 0.0f, state.angularVelocity.z);
            body.force_ = glm::vec3(0.0f);
            body.torque_ = glm::vec3(0.0f);
            body.impulse_ = glm::vec3(0.0f);
            body.angularImpulse_ = glm::vec3(0.0f);
            return;
    }
}

bool World::dynamic(int index) const {
    return index != 0 && states_[static_cast<std::size_t>(index)].body->motion_ == RigidBodyComponent::Motion::Dynamic;
}

void World::detect(float dt) {
    bounds_.clear();
    for (const Proxy& proxy : proxies_) bounds_.push_back(proxy.bounds);
    broadphase_.build(bounds_);

    Manifold manifold;
    for (std::size_t i = 0; i < proxies_.size(); ++i) {
        const Proxy& a = proxies_[i];
        broadphase_.query(a.bounds, nearby_);
        std::sort(nearby_.begin(), nearby_.end());

        for (const int index : nearby_) {
            const std::size_t j = static_cast<std::size_t>(index);
            if (j <= i) continue;

            const Proxy& b = proxies_[j];
            if (a.owner == b.owner) continue;
            if (!dynamic(a.owner) && !dynamic(b.owner)) continue;
            if (!collide(a.geometry, b.geometry, manifold)) continue;

            const RigidBodyComponent* first = states_[static_cast<std::size_t>(a.owner)].body;
            const RigidBodyComponent* second = states_[static_cast<std::size_t>(b.owner)].body;
            touching_.push_back({contactKey(a.collider->id(), b.collider->id(), 0),
                                 (first != nullptr ? first->owner() : a.collider->owner())->id(),
                                 (second != nullptr ? second->owner() : b.collider->owner())->id(),
                                 a.collider->id(), b.collider->id()});

            for (int k = 0; k < manifold.count; ++k) {
                const ContactPoint& point = manifold.points[static_cast<std::size_t>(k)];
                solver_.add(a.owner, b.owner, contactKey(a.collider->id(), b.collider->id(), point.feature),
                            manifold.normal, point,
                            std::sqrt(a.collider->friction() * b.collider->friction()),
                            std::max(a.collider->restitution(), b.collider->restitution()), dt);
            }
        }
    }
}

int World::island(int index) {
    int root = index;
    while (islands_[static_cast<std::size_t>(root)] != root) {
        root = islands_[static_cast<std::size_t>(root)];
    }
    while (islands_[static_cast<std::size_t>(index)] != root) {
        const int next = islands_[static_cast<std::size_t>(index)];
        islands_[static_cast<std::size_t>(index)] = root;
        index = next;
    }
    return root;
}

bool World::stirring(int index) const {
    if (index == 0) return false;

    const BodyState& state = states_[static_cast<std::size_t>(index)];
    return glm::length(state.velocity) >= SLEEP_LINEAR ||
           glm::length(state.angularVelocity) >= SLEEP_ANGULAR;
}

void World::rest(float dt) {
    islands_.resize(states_.size());
    settled_.assign(states_.size(), 1);
    nudged_.assign(states_.size(), 0);
    for (std::size_t i = 0; i < states_.size(); ++i) islands_[i] = static_cast<int>(i);

    for (const auto& contact : solver_.contacts()) {
        const bool left = dynamic(contact.a);
        const bool right = dynamic(contact.b);

        if (left && right) {
            const int one = island(contact.a);
            const int other = island(contact.b);
            if (one != other) islands_[static_cast<std::size_t>(one)] = other;
            continue;
        }
        if (left && stirring(contact.b)) nudged_[static_cast<std::size_t>(contact.a)] = 1;
        if (right && stirring(contact.a)) nudged_[static_cast<std::size_t>(contact.b)] = 1;
    }

    for (std::size_t i = 1; i < states_.size(); ++i) {
        if (!dynamic(static_cast<int>(i))) continue;

        BodyState& state = states_[i];
        RigidBodyComponent& body = *state.body;
        const bool slow = nudged_[i] == 0 && glm::length(state.velocity) < SLEEP_LINEAR &&
                          glm::length(state.angularVelocity) < SLEEP_ANGULAR;
        body.sleepTimer_ = slow ? body.sleepTimer_ + dt : 0.0f;
        if (body.sleepTimer_ < SLEEP_TIME) {
            settled_[static_cast<std::size_t>(island(static_cast<int>(i)))] = 0;
        }
    }

    for (std::size_t i = 1; i < states_.size(); ++i) {
        if (!dynamic(static_cast<int>(i))) continue;
        states_[i].body->asleep_ = settled_[static_cast<std::size_t>(island(static_cast<int>(i)))] != 0;
    }
}

void World::advance(BodyState& state, float dt) {
    RigidBodyComponent& body = *state.body;
    body.simulated_ = true;
    body.center_ = state.center;

    if (body.motion_ != RigidBodyComponent::Motion::Dynamic) {
        body.placed_ = body.transform()->world();
        return;
    }

    body.velocity_ = body.asleep_ ? glm::vec3(0.0f) : state.velocity;
    body.angularVelocity_ = body.asleep_ ? glm::vec3(0.0f) : state.angularVelocity;
    body.wroteVelocity_ = body.velocity_;
    body.wroteSpin_ = body.angularVelocity_;
    if (body.asleep_) return;

    const glm::vec3 turn = state.angularVelocity + state.angularDrift;
    const float speed = glm::length(turn);
    const glm::quat delta =
        speed > 0.0f ? glm::angleAxis(speed * dt, turn / speed) : glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    const glm::quat turned = speed > 0.0f ? glm::normalize(delta * body.orientation_) : body.orientation_;
    const glm::vec3 center = state.center + (state.velocity + state.drift) * dt;

    body.origin_ = center + delta * (body.origin_ - state.center);
    body.orientation_ = turned;
    body.center_ = center;
    write(body);
}

void World::write(RigidBodyComponent& body) {
    Transform& transform = *body.transform();
    glm::vec3 position = body.origin_;
    glm::quat orientation = body.orientation_;

    if (Transform* above = body.parentTransform()) {
        const glm::mat4& parentWorld = above->world();
        position = glm::vec3(glm::inverse(parentWorld) * glm::vec4(position, 1.0f));
        orientation = glm::conjugate(orientationOf(parentWorld)) * orientation;
    }

    const glm::vec3 rotation = rotationOf(orientation);
    transform.setPosition(position.x, position.y, position.z);
    transform.setRotation(rotation.x, rotation.y, rotation.z);
    body.placed_ = transform.world();
}

}
