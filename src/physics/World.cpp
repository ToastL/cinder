#include "physics/World.hpp"

#include "physics/Body.hpp"
#include "physics/Collide.hpp"
#include "physics/Collider.hpp"
#include "physics/Pose.hpp"
#include "lua/LuaApi.hpp"
#include "scene/Node.hpp"
#include "scene/Scene.hpp"
#include "scene/Transform.hpp"

#include <glm/geometric.hpp>
#include <glm/matrix.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace cinder::physics {

using cinder::scene::Node;
using cinder::scene::Transform;

namespace {

std::uint64_t contactKey(int a, int b, int feature) {
    return (static_cast<std::uint64_t>(a & 0x3FFFFF) << 42) |
           (static_cast<std::uint64_t>(b & 0x3FFFFF) << 20) |
           static_cast<std::uint64_t>(feature & 0xFFFFF);
}

World& hostOf(lua_State* state) { return *cinder::lua::LuaApi::context<World>(state); }

glm::vec3 vectorAt(lua_State* state, int index) {
    return glm::vec3(static_cast<float>(lua_tonumber(state, index)),
                     static_cast<float>(lua_tonumber(state, index + 1)),
                     static_cast<float>(lua_tonumber(state, index + 2)));
}

Body* bodyAt(lua_State* state, int index) {
    const int id = static_cast<int>(lua_tointeger(state, index));
    return dynamic_cast<Body*>(hostOf(state).scene().byId(id));
}

int applyImpulse(lua_State* state) {
    Body* body = bodyAt(state, 1);
    if (body == nullptr) return luaL_error(state, "applyImpulse: the node is not a Body");

    if (lua_gettop(state) >= 7) body->applyImpulse(vectorAt(state, 2), vectorAt(state, 5));
    else body->applyImpulse(vectorAt(state, 2));
    return 0;
}

int applyForce(lua_State* state) {
    Body* body = bodyAt(state, 1);
    if (body == nullptr) return luaL_error(state, "applyForce: the node is not a Body");

    if (lua_gettop(state) >= 7) body->applyForce(vectorAt(state, 2), vectorAt(state, 5));
    else body->applyForce(vectorAt(state, 2));
    return 0;
}

int applyTorque(lua_State* state) {
    Body* body = bodyAt(state, 1);
    if (body == nullptr) return luaL_error(state, "applyTorque: the node is not a Body");

    body->applyTorque(vectorAt(state, 2));
    return 0;
}

int castRay(lua_State* state) {
    RayHit hit;
    const float reach = cinder::lua::LuaApi::optFloat(state, 7, 1000.0f);
    if (!hostOf(state).raycast(vectorAt(state, 1), vectorAt(state, 4), reach, hit)) return 0;

    lua_pushinteger(state, hit.node->id());
    lua_pushnumber(state, hit.position.x);
    lua_pushnumber(state, hit.position.y);
    lua_pushnumber(state, hit.position.z);
    lua_pushnumber(state, hit.normal.x);
    lua_pushnumber(state, hit.normal.y);
    lua_pushnumber(state, hit.normal.z);
    lua_pushnumber(state, hit.distance);
    return 8;
}

int readGravity(lua_State* state) {
    const glm::vec3& gravity = hostOf(state).gravity();
    lua_pushnumber(state, gravity.x);
    lua_pushnumber(state, gravity.y);
    lua_pushnumber(state, gravity.z);
    return 3;
}

int writeGravity(lua_State* state) {
    hostOf(state).setGravity(vectorAt(state, 1));
    return 0;
}

std::array<glm::vec3, 2> basisAround(const glm::vec3& normal) {
    const glm::vec3 guide = std::abs(normal.x) >= 0.57735f ? glm::vec3(normal.y, -normal.x, 0.0f)
                                                           : glm::vec3(0.0f, normal.z, -normal.y);
    const glm::vec3 first = glm::normalize(guide);
    return {first, glm::cross(normal, first)};
}

}

void World::step(float dt) {
    if (dt <= 0.0f) return;

    states_.clear();
    proxies_.clear();
    contacts_.clear();
    touching_.clear();
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
    warmStart();
    solve();
    rest(dt);
    for (std::size_t i = 1; i < states_.size(); ++i) advance(states_[i], dt);
    notify();
}

void World::notify() {
    current_.clear();
    for (const Touch& touch : touching_) current_.insert(touch.key);

    if (observer_ != nullptr) {
        for (const Touch& touch : touching_) {
            if (previous_.find(touch.key) != previous_.end()) continue;
            Node* a = scene_.byId(touch.a);
            Node* b = scene_.byId(touch.b);
            if (a != nullptr && b != nullptr) observer_->touched(*a, *b);
        }

        for (const Touch& touch : touched_) {
            if (current_.find(touch.key) != current_.end()) continue;
            Node* a = scene_.byId(touch.a);
            Node* b = scene_.byId(touch.b);
            if (a != nullptr && b != nullptr) observer_->touchEnded(*a, *b);
        }
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

void World::registerApi(cinder::lua::LuaApi& api) {
    api.bind("applyImpulse", applyImpulse, this);
    api.bind("applyForce", applyForce, this);
    api.bind("applyTorque", applyTorque, this);
    api.bind("raycast", castRay, this);
    api.bind("gravity", readGravity, this);
    api.bind("setGravity", writeGravity, this);
}

bool World::raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance,
                    RayHit& hit) const {
    const float reach = glm::length(direction);
    if (reach <= 0.0f || maxDistance <= 0.0f) return false;

    const glm::vec3 heading = direction / reach;
    bool found = false;
    for (Node* root : scene_.roots()) cast(*root, nullptr, origin, heading, maxDistance, hit, found);
    return found;
}

void World::cast(Node& node, Body* owner, const glm::vec3& origin, const glm::vec3& direction,
                 float maxDistance, RayHit& hit, bool& found) const {
    if (node.destroyed() || !node.isEnabled()) return;

    if (auto* body = dynamic_cast<Body*>(&node)) {
        owner = body;
    } else if (auto* collider = dynamic_cast<Collider*>(&node)) {
        float distance = 0.0f;
        glm::vec3 normal(0.0f);
        const Geometry geometry = collider->geometry();
        if (rayHits(geometry, origin, direction, distance, normal) && distance <= maxDistance &&
            (!found || distance < hit.distance)) {
            hit.node = owner != nullptr ? static_cast<Node*>(owner) : static_cast<Node*>(collider);
            hit.collider = collider;
            hit.position = origin + direction * distance;
            hit.normal = normal;
            hit.distance = distance;
            found = true;
        }
    }

    for (Node* child : node.children()) cast(*child, owner, origin, direction, maxDistance, hit, found);
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
    if (body.velocity_ != body.wroteVelocity_ || body.angularVelocity_ != body.wroteSpin_) body.wake();

    const glm::mat4& world = body.transform()->world();
    if (body.motion_ == Body::Motion::Dynamic && body.simulated_ && world == body.placed_) return;

    body.wake();

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
        if (state.body->asleep_) continue;
        state.inverseMass = 1.0f / state.body->mass_;
        if (glm::determinant(state.inertia) > 0.0f) state.inverseInertia = glm::inverse(state.inertia);
        if (!state.body->planar_) continue;

        state.linearMask = glm::vec3(1.0f, 1.0f, 0.0f);
        const glm::mat3 spin(glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        state.inverseInertia = spin * state.inverseInertia * spin;
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
    return index != 0 && states_[static_cast<std::size_t>(index)].body->motion_ == Body::Motion::Dynamic;
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

            const Body* first = states_[static_cast<std::size_t>(a.owner)].body;
            const Body* second = states_[static_cast<std::size_t>(b.owner)].body;
            touching_.push_back({contactKey(a.collider->id(), b.collider->id(), 0),
                                 first != nullptr ? first->id() : a.collider->id(),
                                 second != nullptr ? second->id() : b.collider->id()});

            for (int k = 0; k < manifold.count; ++k) {
                addContact(a, b, manifold.normal, manifold.points[static_cast<std::size_t>(k)], dt);
            }
        }
    }
}

void World::addContact(const Proxy& a, const Proxy& b, const glm::vec3& normal,
                       const ContactPoint& point, float dt) {
    const State& first = states_[static_cast<std::size_t>(a.owner)];
    const State& second = states_[static_cast<std::size_t>(b.owner)];

    Contact contact;
    contact.key = contactKey(a.collider->id(), b.collider->id(), point.feature);
    contact.a = a.owner;
    contact.b = b.owner;
    contact.normal = normal;
    contact.tangents = basisAround(normal);
    contact.fromA = point.position - first.center;
    contact.fromB = point.position - second.center;
    contact.friction = std::sqrt(a.collider->friction() * b.collider->friction());

    contact.normalMass = effectiveMass(first, second, contact.fromA, contact.fromB, normal);
    for (int i = 0; i < 2; ++i) {
        const std::size_t index = static_cast<std::size_t>(i);
        contact.tangentMass[index] =
            effectiveMass(first, second, contact.fromA, contact.fromB, contact.tangents[index]);
    }

    const float restitution = std::max(a.collider->restitution(), b.collider->restitution());
    const float closing = glm::dot(approach(contact), normal);
    contact.push = BAUMGARTE / dt * std::max(point.depth - SLOP, 0.0f);
    contact.bounce = closing < -BOUNCE_THRESHOLD ? -restitution * closing : 0.0f;

    contacts_.push_back(contact);
}

void World::warmStart() {
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

void World::apply(const Contact& contact, const glm::vec3& impulse) {
    State& a = states_[static_cast<std::size_t>(contact.a)];
    State& b = states_[static_cast<std::size_t>(contact.b)];
    a.velocity -= a.inverseMass * (impulse * a.linearMask);
    a.angularVelocity -= a.inverseInertia * glm::cross(contact.fromA, impulse);
    b.velocity += b.inverseMass * (impulse * b.linearMask);
    b.angularVelocity += b.inverseInertia * glm::cross(contact.fromB, impulse);
}

void World::applyDrift(const Contact& contact, const glm::vec3& impulse) {
    State& a = states_[static_cast<std::size_t>(contact.a)];
    State& b = states_[static_cast<std::size_t>(contact.b)];
    a.drift -= a.inverseMass * (impulse * a.linearMask);
    a.angularDrift -= a.inverseInertia * glm::cross(contact.fromA, impulse);
    b.drift += b.inverseMass * (impulse * b.linearMask);
    b.angularDrift += b.inverseInertia * glm::cross(contact.fromB, impulse);
}

float World::effectiveMass(const State& a, const State& b, const glm::vec3& fromA,
                           const glm::vec3& fromB, const glm::vec3& direction) const {
    const glm::vec3 turnA = glm::cross(fromA, direction);
    const glm::vec3 turnB = glm::cross(fromB, direction);
    const float stiffness = a.inverseMass * glm::dot(direction, a.linearMask * direction) +
                            b.inverseMass * glm::dot(direction, b.linearMask * direction) +
                            glm::dot(turnA, a.inverseInertia * turnA) +
                            glm::dot(turnB, b.inverseInertia * turnB);
    return stiffness > 0.0f ? 1.0f / stiffness : 0.0f;
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

    const State& state = states_[static_cast<std::size_t>(index)];
    return glm::length(state.velocity) >= SLEEP_LINEAR ||
           glm::length(state.angularVelocity) >= SLEEP_ANGULAR;
}

void World::rest(float dt) {
    islands_.resize(states_.size());
    settled_.assign(states_.size(), 1);
    nudged_.assign(states_.size(), 0);
    for (std::size_t i = 0; i < states_.size(); ++i) islands_[i] = static_cast<int>(i);

    for (const Contact& contact : contacts_) {
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

        State& state = states_[i];
        Body& body = *state.body;
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

glm::vec3 World::approach(const Contact& contact) const {
    const State& a = states_[static_cast<std::size_t>(contact.a)];
    const State& b = states_[static_cast<std::size_t>(contact.b)];
    return b.velocity + glm::cross(b.angularVelocity, contact.fromB) - a.velocity -
           glm::cross(a.angularVelocity, contact.fromA);
}

glm::vec3 World::driftApproach(const Contact& contact) const {
    const State& a = states_[static_cast<std::size_t>(contact.a)];
    const State& b = states_[static_cast<std::size_t>(contact.b)];
    return b.drift + glm::cross(b.angularDrift, contact.fromB) - a.drift -
           glm::cross(a.angularDrift, contact.fromA);
}

void World::solveFriction(Contact& contact) {
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

void World::solveNormal(Contact& contact) {
    const float closing = glm::dot(approach(contact), contact.normal);
    const float previous = contact.impulse;
    contact.impulse = std::max(previous + contact.normalMass * (contact.bounce - closing), 0.0f);
    apply(contact, contact.normal * (contact.impulse - previous));
}

void World::solvePush(Contact& contact) {
    if (contact.push <= 0.0f) return;

    const float closing = glm::dot(driftApproach(contact), contact.normal);
    const float previous = contact.pushImpulse;
    contact.pushImpulse = std::max(previous + contact.normalMass * (contact.push - closing), 0.0f);
    applyDrift(contact, contact.normal * (contact.pushImpulse - previous));
}

void World::solve() {
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

void World::advance(State& state, float dt) {
    Body& body = *state.body;
    body.simulated_ = true;
    body.center_ = state.center;

    if (body.motion_ != Body::Motion::Dynamic) {
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
