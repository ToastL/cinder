#pragma once

#include "physics/Geometry.hpp"

#include <glm/mat3x3.hpp>
#include <glm/vec3.hpp>

#include <array>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace cinder::lua { class LuaApi; }

namespace cinder::scene {
class Node;
class Scene;
}

namespace cinder::physics {

class Body;
class Collider;
struct ContactPoint;

class ContactObserver {
public:
    virtual ~ContactObserver() = default;
    virtual void touched(cinder::scene::Node& a, cinder::scene::Node& b) = 0;
    virtual void touchEnded(cinder::scene::Node& a, cinder::scene::Node& b) = 0;
};

struct RayHit {
    cinder::scene::Node* node = nullptr;
    Collider* collider = nullptr;
    glm::vec3 position{0.0f};
    glm::vec3 normal{0.0f};
    float distance = 0.0f;
};

class World {
public:
    static constexpr int ITERATIONS = 10;
    static constexpr float SLOP = 0.005f;
    static constexpr float BAUMGARTE = 0.2f;
    static constexpr float BOUNCE_THRESHOLD = 1.0f;

    explicit World(cinder::scene::Scene& scene) : scene_(scene) {}

    World(const World&) = delete;
    World& operator=(const World&) = delete;

    cinder::scene::Scene& scene() const { return scene_; }

    const glm::vec3& gravity() const { return gravity_; }
    void setGravity(const glm::vec3& gravity) { gravity_ = gravity; }

    void setObserver(ContactObserver* observer) { observer_ = observer; }

    void step(float dt);
    bool raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance,
                 RayHit& hit) const;
    void registerApi(cinder::lua::LuaApi& api);

private:
    struct State {
        Body* body = nullptr;
        glm::vec3 center{0.0f};
        glm::vec3 velocity{0.0f};
        glm::vec3 angularVelocity{0.0f};
        glm::vec3 drift{0.0f};
        glm::vec3 angularDrift{0.0f};
        glm::vec3 linearMask{1.0f};
        float inverseMass = 0.0f;
        glm::mat3 inverseInertia{0.0f};
        float volume = 0.0f;
        glm::vec3 moment{0.0f};
        glm::mat3 inertia{0.0f};
    };

    struct Proxy {
        Collider* collider = nullptr;
        int owner = 0;
        Geometry geometry;
        Bounds bounds;
    };

    struct Contact {
        std::uint64_t key = 0;
        int a = 0;
        int b = 0;
        glm::vec3 normal{0.0f};
        std::array<glm::vec3, 2> tangents{};
        glm::vec3 fromA{0.0f};
        glm::vec3 fromB{0.0f};
        float normalMass = 0.0f;
        std::array<float, 2> tangentMass{};
        float bounce = 0.0f;
        float push = 0.0f;
        float friction = 0.0f;
        float impulse = 0.0f;
        float pushImpulse = 0.0f;
        std::array<float, 2> tangentImpulse{};
    };

    struct Impulses {
        float normal = 0.0f;
        std::array<float, 2> tangent{};
    };

    struct Touch {
        std::uint64_t key = 0;
        int a = 0;
        int b = 0;
    };

    void gather(cinder::scene::Node& node, int owner);
    void cast(cinder::scene::Node& node, Body* owner, const glm::vec3& origin,
              const glm::vec3& direction, float maxDistance, RayHit& hit, bool& found) const;
    void notify();
    void place(State& state, float dt);
    void weigh();
    void accelerate(State& state, float dt);
    void detect(float dt);
    void addContact(const Proxy& a, const Proxy& b, const glm::vec3& normal,
                    const ContactPoint& point, float dt);
    void warmStart();
    void solve();
    void solveFriction(Contact& contact);
    void solveNormal(Contact& contact);
    void solvePush(Contact& contact);
    void apply(const Contact& contact, const glm::vec3& impulse);
    void applyDrift(const Contact& contact, const glm::vec3& impulse);
    void advance(State& state, float dt);
    void write(Body& body);
    bool dynamic(int index) const;
    float effectiveMass(const State& a, const State& b, const glm::vec3& fromA,
                        const glm::vec3& fromB, const glm::vec3& direction) const;
    glm::vec3 approach(const Contact& contact) const;
    glm::vec3 driftApproach(const Contact& contact) const;

    cinder::scene::Scene& scene_;
    glm::vec3 gravity_{0.0f, -9.81f, 0.0f};
    std::vector<State> states_;
    std::vector<Proxy> proxies_;
    std::vector<Contact> contacts_;
    std::unordered_map<std::uint64_t, Impulses> cached_;
    std::unordered_map<std::uint64_t, Impulses> carried_;
    std::vector<Touch> touching_;
    std::vector<Touch> touched_;
    std::unordered_set<std::uint64_t> current_;
    std::unordered_set<std::uint64_t> previous_;
    ContactObserver* observer_ = nullptr;
};

}
