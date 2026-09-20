#include "physics/World.hpp"

#include "physics/Body.hpp"
#include "physics/Collide.hpp"
#include "physics/Collider.hpp"
#include "physics/Pose.hpp"
#include "scene/Node.hpp"
#include "scene/Scene.hpp"
#include "scene/Transform.hpp"

#include <glm/geometric.hpp>
#include <glm/matrix.hpp>

#include <algorithm>
#include <cstddef>

namespace cinder::physics {

using cinder::scene::Node;
using cinder::scene::Transform;

void World::step(float dt) {
    if (dt <= 0.0f) return;

    states_.clear();
    proxies_.clear();
    contacts_.clear();
    states_.emplace_back();

    for (Node* root : scene_.roots()) gather(*root, 0);
    for (std::size_t i = 1; i < states_.size(); ++i) place(states_[i], dt);
    for (Proxy& proxy : proxies_) {
        proxy.geometry = proxy.collider->geometry();
        proxy.bounds = boundsOf(proxy.geometry);
    }
    weigh();
    for (std::size_t i = 1; i < states_.size(); ++i) accelerate(states_[i], dt);
    detect(dt);
    solve();
    for (std::size_t i = 1; i < states_.size(); ++i) advance(states_[i], dt);
}

void World::gather(Node& node, int owner) {
    if (node.destroyed() || !node.isEnabled()) return;

    if (auto* body = dynamic_cast<Body*>(&node)) {
        State state;
        state.body = body;
        states_.push_back(state);
        owner = static_cast<int>(states_.size()) - 1;
    } else if (auto* collider = dynamic_cast<Collider*>(&node)) {
        Proxy proxy;
        proxy.collider = collider;
        proxy.owner = owner;
        proxies_.push_back(proxy);
    }

    for (Node* child : node.children()) gather(*child, owner);
}

void World::place(State& state, float dt) {
    Body& body = *state.body;
    const glm::mat4& world = body.transform()->world();
    if (body.motion_ == Body::Motion::Dynamic && body.simulated_ && world == body.placed_) return;

    const glm::quat previous = body.orientation_;
    body.origin_ = glm::vec3(world[3]);
    body.orientation_ = orientationOf(world);
    if (body.motion_ == Body::Motion::Kinematic) {
        body.angularVelocity_ =
            body.simulated_ ? angularVelocityOf(previous, body.orientation_, dt) : glm::vec3(0.0f);
    }
}

void World::weigh() {
    for (const Proxy& proxy : proxies_) {
        if (proxy.owner == 0) continue;
        State& state = states_[static_cast<std::size_t>(proxy.owner)];
        const float volume = volumeOf(proxy.geometry);
        state.volume += volume;
        state.moment += proxy.geometry.center * volume;
    }

    for (std::size_t i = 1; i < states_.size(); ++i) {
        State& state = states_[i];
        state.center = state.volume > 0.0f ? state.moment / state.volume : state.body->origin_;
    }

    for (const Proxy& proxy : proxies_) {
        if (proxy.owner == 0) continue;
        State& state = states_[static_cast<std::size_t>(proxy.owner)];
        if (state.volume <= 0.0f) continue;

        const float mass = state.body->mass_ * volumeOf(proxy.geometry) / state.volume;
        const glm::vec3 offset = proxy.geometry.center - state.center;
        state.inertia += inertiaOf(proxy.geometry, mass) +
                         mass * (glm::dot(offset, offset) * glm::mat3(1.0f) - glm::outerProduct(offset, offset));
    }

    for (std::size_t i = 1; i < states_.size(); ++i) {
        State& state = states_[i];
        if (state.body->motion_ != Body::Motion::Dynamic) continue;
        state.inverseMass = 1.0f / state.body->mass_;
        if (glm::determinant(state.inertia) > 0.0f) state.inverseInertia = glm::inverse(state.inertia);
    }
}

void World::accelerate(State& state, float dt) {
    Body& body = *state.body;
    switch (body.motion_) {
        case Body::Motion::Static:
            return;
        case Body::Motion::Kinematic:
            body.velocity_ = body.simulated_ ? (state.center - body.center_) / dt : glm::vec3(0.0f);
            state.velocity = body.velocity_;
            state.angularVelocity = body.angularVelocity_;
            return;
        case Body::Motion::Dynamic:
            state.velocity = (body.velocity_ + gravity_ * (body.gravityScale_ * dt)) /
                             (1.0f + dt * body.linearDamping_);
            state.angularVelocity = body.angularVelocity_ / (1.0f + dt * body.angularDamping_);
            return;
    }
}

bool World::dynamic(int index) const {
    return index != 0 && states_[static_cast<std::size_t>(index)].body->motion_ == Body::Motion::Dynamic;
}

void World::detect(float dt) {
    Manifold manifold;
    for (std::size_t i = 0; i < proxies_.size(); ++i) {
        const Proxy& a = proxies_[i];
        for (std::size_t j = i + 1; j < proxies_.size(); ++j) {
            const Proxy& b = proxies_[j];
            if (a.owner == b.owner) continue;
            if (!dynamic(a.owner) && !dynamic(b.owner)) continue;
            if (!overlaps(a.bounds, b.bounds)) continue;
            if (!collide(a.geometry, b.geometry, manifold)) continue;

            const float restitution = std::max(a.collider->restitution(), b.collider->restitution());
            for (int k = 0; k < manifold.count; ++k) {
                addContact(a.owner, b.owner, manifold.normal, manifold.points[static_cast<std::size_t>(k)],
                           restitution, dt);
            }
        }
    }
}

void World::addContact(int a, int b, const glm::vec3& normal, const ContactPoint& point,
                       float restitution, float dt) {
    const State& first = states_[static_cast<std::size_t>(a)];
    const State& second = states_[static_cast<std::size_t>(b)];

    Contact contact;
    contact.a = a;
    contact.b = b;
    contact.normal = normal;
    contact.fromA = point.position - first.center;
    contact.fromB = point.position - second.center;

    const glm::vec3 turnA = glm::cross(contact.fromA, normal);
    const glm::vec3 turnB = glm::cross(contact.fromB, normal);
    const float stiffness = first.inverseMass + second.inverseMass +
                            glm::dot(turnA, first.inverseInertia * turnA) +
                            glm::dot(turnB, second.inverseInertia * turnB);
    contact.normalMass = stiffness > 0.0f ? 1.0f / stiffness : 0.0f;

    const float closing = glm::dot(approach(contact), normal);
    const float push = BAUMGARTE / dt * std::max(point.depth - SLOP, 0.0f);
    const float bounce = closing < -BOUNCE_THRESHOLD ? -restitution * closing : 0.0f;
    contact.bias = std::max(push, bounce);

    contacts_.push_back(contact);
}

glm::vec3 World::approach(const Contact& contact) const {
    const State& a = states_[static_cast<std::size_t>(contact.a)];
    const State& b = states_[static_cast<std::size_t>(contact.b)];
    return b.velocity + glm::cross(b.angularVelocity, contact.fromB) - a.velocity -
           glm::cross(a.angularVelocity, contact.fromA);
}

void World::solve() {
    for (int iteration = 0; iteration < ITERATIONS; ++iteration) {
        for (Contact& contact : contacts_) {
            const float closing = glm::dot(approach(contact), contact.normal);
            const float previous = contact.impulse;
            contact.impulse = std::max(previous + contact.normalMass * (contact.bias - closing), 0.0f);

            const glm::vec3 impulse = contact.normal * (contact.impulse - previous);
            State& a = states_[static_cast<std::size_t>(contact.a)];
            State& b = states_[static_cast<std::size_t>(contact.b)];
            a.velocity -= a.inverseMass * impulse;
            a.angularVelocity -= a.inverseInertia * glm::cross(contact.fromA, impulse);
            b.velocity += b.inverseMass * impulse;
            b.angularVelocity += b.inverseInertia * glm::cross(contact.fromB, impulse);
        }
    }
}

void World::advance(State& state, float dt) {
    Body& body = *state.body;
    body.simulated_ = true;
    body.center_ = state.center;

    if (body.motion_ != Body::Motion::Dynamic) {
        body.placed_ = body.transform()->world();
        return;
    }

    body.velocity_ = state.velocity;
    body.angularVelocity_ = state.angularVelocity;

    const float speed = glm::length(state.angularVelocity);
    const glm::quat delta = speed > 0.0f ? glm::angleAxis(speed * dt, state.angularVelocity / speed)
                                         : glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    const glm::quat turned = speed > 0.0f ? glm::normalize(delta * body.orientation_) : body.orientation_;
    const glm::vec3 center = state.center + state.velocity * dt;

    body.origin_ = center + delta * (body.origin_ - state.center);
    body.orientation_ = turned;
    body.center_ = center;
    write(body);
}

void World::write(Body& body) {
    Transform& transform = *body.transform();
    glm::vec3 position = body.origin_;
    glm::quat orientation = body.orientation_;

    Node* parent = body.parent();
    if (Transform* above = parent != nullptr ? parent->transform() : nullptr) {
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
