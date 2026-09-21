#include "physics/ContactSolver.hpp"

#include "physics/Collide.hpp"

#include <glm/geometric.hpp>

#include <algorithm>
#include <cmath>

namespace cinder::physics {
namespace {

std::array<glm::vec3, 2> basisAround(const glm::vec3& normal) {
    const glm::vec3 guide = std::abs(normal.x) >= 0.57735f ? glm::vec3(normal.y, -normal.x, 0.0f)
                                                           : glm::vec3(0.0f, normal.z, -normal.y);
    const glm::vec3 first = glm::normalize(guide);
    return {first, glm::cross(normal, first)};
}

}

void ContactSolver::begin(std::span<BodyState> states) {
    states_ = states;
    contacts_.clear();
}

void ContactSolver::add(int a, int b, std::uint64_t key, const glm::vec3& normal,
                        const ContactPoint& point, float friction, float restitution, float dt) {
    const BodyState& first = states_[static_cast<std::size_t>(a)];
    const BodyState& second = states_[static_cast<std::size_t>(b)];

    Contact contact;
    contact.key = key;
    contact.a = a;
    contact.b = b;
    contact.normal = normal;
    contact.tangents = basisAround(normal);
    contact.fromA = point.position - first.center;
    contact.fromB = point.position - second.center;
    contact.friction = friction;

    contact.normalMass = effectiveMass(first, second, contact.fromA, contact.fromB, normal);
    for (int i = 0; i < 2; ++i) {
        const std::size_t index = static_cast<std::size_t>(i);
        contact.tangentMass[index] =
            effectiveMass(first, second, contact.fromA, contact.fromB, contact.tangents[index]);
    }

    const float closing = glm::dot(approach(contact), normal);
    contact.push = BAUMGARTE / dt * std::max(point.depth - SLOP, 0.0f);
    contact.bounce = closing < -BOUNCE_THRESHOLD ? -restitution * closing : 0.0f;

    contacts_.push_back(contact);
}

void ContactSolver::warmStart() {
    for (Contact& contact : contacts_) {
        const auto found = carried_.find(contact.key);
        if (found != carried_.end()) {
            contact.impulse = found->second.normal;
            contact.tangentImpulse = found->second.tangent;
        }

        apply(contact, contact.normal * contact.impulse +
                           contact.tangents[0] * contact.tangentImpulse[0] +
                           contact.tangents[1] * contact.tangentImpulse[1]);
    }
}

void ContactSolver::apply(const Contact& contact, const glm::vec3& impulse) {
    BodyState& a = states_[static_cast<std::size_t>(contact.a)];
    BodyState& b = states_[static_cast<std::size_t>(contact.b)];
    a.velocity -= a.inverseMass * (impulse * a.linearMask);
    a.angularVelocity -= a.inverseInertia * glm::cross(contact.fromA, impulse);
    b.velocity += b.inverseMass * (impulse * b.linearMask);
    b.angularVelocity += b.inverseInertia * glm::cross(contact.fromB, impulse);
}

void ContactSolver::applyDrift(const Contact& contact, const glm::vec3& impulse) {
    BodyState& a = states_[static_cast<std::size_t>(contact.a)];
    BodyState& b = states_[static_cast<std::size_t>(contact.b)];
    a.drift -= a.inverseMass * (impulse * a.linearMask);
    a.angularDrift -= a.inverseInertia * glm::cross(contact.fromA, impulse);
    b.drift += b.inverseMass * (impulse * b.linearMask);
    b.angularDrift += b.inverseInertia * glm::cross(contact.fromB, impulse);
}

float ContactSolver::effectiveMass(const BodyState& a, const BodyState& b, const glm::vec3& fromA,
                           const glm::vec3& fromB, const glm::vec3& direction) const {
    const glm::vec3 turnA = glm::cross(fromA, direction);
    const glm::vec3 turnB = glm::cross(fromB, direction);
    const float stiffness = a.inverseMass * glm::dot(direction, a.linearMask * direction) +
                            b.inverseMass * glm::dot(direction, b.linearMask * direction) +
                            glm::dot(turnA, a.inverseInertia * turnA) +
                            glm::dot(turnB, b.inverseInertia * turnB);
    return stiffness > 0.0f ? 1.0f / stiffness : 0.0f;
}

glm::vec3 ContactSolver::approach(const Contact& contact) const {
    const BodyState& a = states_[static_cast<std::size_t>(contact.a)];
    const BodyState& b = states_[static_cast<std::size_t>(contact.b)];
    return b.velocity + glm::cross(b.angularVelocity, contact.fromB) - a.velocity -
           glm::cross(a.angularVelocity, contact.fromA);
}

glm::vec3 ContactSolver::driftApproach(const Contact& contact) const {
    const BodyState& a = states_[static_cast<std::size_t>(contact.a)];
    const BodyState& b = states_[static_cast<std::size_t>(contact.b)];
    return b.drift + glm::cross(b.angularDrift, contact.fromB) - a.drift -
           glm::cross(a.angularDrift, contact.fromA);
}

void ContactSolver::solveFriction(Contact& contact) {
    const float limit = contact.friction * contact.impulse;
    for (int i = 0; i < 2; ++i) {
        const std::size_t index = static_cast<std::size_t>(i);
        const glm::vec3& tangent = contact.tangents[index];
        const float sliding = glm::dot(approach(contact), tangent);
        const float previous = contact.tangentImpulse[index];
        contact.tangentImpulse[index] =
            std::clamp(previous - contact.tangentMass[index] * sliding, -limit, limit);
        apply(contact, tangent * (contact.tangentImpulse[index] - previous));
    }
}

void ContactSolver::solveNormal(Contact& contact) {
    const float closing = glm::dot(approach(contact), contact.normal);
    const float previous = contact.impulse;
    contact.impulse = std::max(previous + contact.normalMass * (contact.bounce - closing), 0.0f);
    apply(contact, contact.normal * (contact.impulse - previous));
}

void ContactSolver::solvePush(Contact& contact) {
    if (contact.push <= 0.0f) return;

    const float closing = glm::dot(driftApproach(contact), contact.normal);
    const float previous = contact.pushImpulse;
    contact.pushImpulse = std::max(previous + contact.normalMass * (contact.push - closing), 0.0f);
    applyDrift(contact, contact.normal * (contact.pushImpulse - previous));
}

void ContactSolver::solve() {
    for (int iteration = 0; iteration < ITERATIONS; ++iteration) {
        for (Contact& contact : contacts_) {
            solveFriction(contact);
            solveNormal(contact);
        }
    }

    for (int iteration = 0; iteration < ITERATIONS; ++iteration) {
        for (Contact& contact : contacts_) solvePush(contact);
    }

    cached_.clear();
    for (const Contact& contact : contacts_) {
        cached_[contact.key] = {contact.impulse, contact.tangentImpulse};
    }
    carried_.swap(cached_);
}

}
