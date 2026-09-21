#pragma once

#include "physics/Broadphase.hpp"
#include "physics/ContactSolver.hpp"
#include "physics/Geometry.hpp"

#include <glm/vec3.hpp>

#include <cstdint>
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
    static constexpr int ITERATIONS = ContactSolver::ITERATIONS;
    static constexpr float SLOP = ContactSolver::SLOP;
    static constexpr float BAUMGARTE = ContactSolver::BAUMGARTE;
    static constexpr float BOUNCE_THRESHOLD = ContactSolver::BOUNCE_THRESHOLD;
    static constexpr float SLEEP_TIME = 0.5f;
    static constexpr float SLEEP_LINEAR = 0.05f;
    static constexpr float SLEEP_ANGULAR = 0.1f;

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
    struct Proxy {
        Collider* collider = nullptr;
        int owner = 0;
        Geometry geometry;
        Bounds bounds;
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

    cinder::scene::Scene& scene_;
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
    std::unordered_set<std::uint64_t> current_;
    std::unordered_set<std::uint64_t> previous_;
    ContactObserver* observer_ = nullptr;
};

}
