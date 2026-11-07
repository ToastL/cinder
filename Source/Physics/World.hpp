#pragma once

#include "Physics/Broadphase.hpp"
#include "Physics/ContactSolver.hpp"
#include "Physics/Geometry.hpp"
#include "Scene/Scene.hpp"

#include <glm/vec3.hpp>

#include <cstdint>
#include <unordered_set>
#include <vector>

namespace cinder::Physics {

class Body;
class Collider;

struct RayHit {
    cinder::Scene::Node* node = nullptr;
    Collider* collider = nullptr;
    glm::vec3 position{0.0f};
    glm::vec3 normal{0.0f};
    float distance = 0.0f;
};

class World {
public:
    static constexpr float SLEEP_TIME = 0.5f;
    static constexpr float SLEEP_LINEAR = 0.05f;
    static constexpr float SLEEP_ANGULAR = 0.1f;

    explicit World(cinder::Scene::Scene& scene) : scene_(scene) {}

    World(const World&) = delete;
    World& operator=(const World&) = delete;

    cinder::Scene::Scene& scene() const { return scene_; }

    const glm::vec3& gravity() const { return gravity_; }
    void setGravity(const glm::vec3& gravity) { gravity_ = gravity; }

    void step(float dt);
    bool raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance,
                 RayHit& hit) const;

private:
    struct Proxy {
        Collider* collider = nullptr;
        int owner = 0;
        Geometry geometry;
        Bounds bounds;
    };

    struct Touch {
        ContactKey key;
        int a = 0;
        int b = 0;
    };

    void gather(cinder::Scene::Node& node, int owner);
    void cast(cinder::Scene::Node& node, Body* owner, const glm::vec3& origin,
              const glm::vec3& direction, float maxDistance, RayHit& hit, bool& found) const;
    void notify();
    void signal(const Touch& touch, cinder::Scene::Signal signal);
    void place(BodyState& state, float dt);
    void weigh();
    void accelerate(BodyState& state, float dt);
    void detect(float dt);
    void rest(float dt);
    int island(int index);
    bool stirring(int index) const;
    void advance(BodyState& state, float dt);
    void write(Body& body);
    bool dynamic(int index) const;

    cinder::Scene::Scene& scene_;
    glm::vec3 gravity_{0.0f, -9.81f, 0.0f};
    std::vector<BodyState> states_;
    std::vector<Proxy> proxies_;
    ContactSolver solver_;
    Broadphase broadphase_;
    std::vector<Bounds> bounds_;
    std::vector<int> nearby_;
    std::vector<int> islands_;
    std::vector<char> settled_;
    std::vector<char> nudged_;
    std::vector<Touch> touching_;
    std::vector<Touch> touched_;
    std::unordered_set<ContactKey, ContactKeyHash> current_;
    std::unordered_set<ContactKey, ContactKeyHash> previous_;
};

}
