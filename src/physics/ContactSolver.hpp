#pragma once

#include "physics/BodyState.hpp"

#include <array>
#include <cstdint>
#include <span>
#include <unordered_map>
#include <vector>

namespace cinder::physics {

struct ContactPoint;

class ContactSolver {
public:
    static constexpr int ITERATIONS = 10;
    static constexpr float SLOP = 0.005f;
    static constexpr float BAUMGARTE = 0.2f;
    static constexpr float BOUNCE_THRESHOLD = 1.0f;

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

    void begin(std::span<BodyState> states);
    void add(int a, int b, std::uint64_t key, const glm::vec3& normal,
             const ContactPoint& point, float friction, float restitution, float dt);
    void warmStart();
    void solve();
    std::span<const Contact> contacts() const { return contacts_; }

private:
    struct Impulses {
        float normal = 0.0f;
        std::array<float, 2> tangent{};
    };

    void solveFriction(Contact& contact);
    void solveNormal(Contact& contact);
    void solvePush(Contact& contact);
    void apply(const Contact& contact, const glm::vec3& impulse);
    void applyDrift(const Contact& contact, const glm::vec3& impulse);
    float effectiveMass(const BodyState& a, const BodyState& b, const glm::vec3& fromA,
                        const glm::vec3& fromB, const glm::vec3& direction) const;
    glm::vec3 approach(const Contact& contact) const;
    glm::vec3 driftApproach(const Contact& contact) const;

    std::span<BodyState> states_;
    std::vector<Contact> contacts_;
    std::unordered_map<std::uint64_t, Impulses> cached_;
    std::unordered_map<std::uint64_t, Impulses> carried_;
};

}
