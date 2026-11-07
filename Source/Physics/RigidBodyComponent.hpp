#pragma once

#include "Reflect/Enum.hpp"
#include "Scene/Component.hpp"

#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <algorithm>
#include <cfloat>

namespace cinder::Scene { class Transform; }

namespace cinder::Physics {

class RigidBodyComponent final : public cinder::Scene::Component {
public:
    static constexpr float MIN_MASS = 0.001f;

    enum class Motion { Static, Kinematic, Dynamic };

    Motion motion() const { return motion_; }
    float mass() const { return mass_; }
    float gravityScale() const { return gravityScale_; }
    float linearDamping() const { return linearDamping_; }
    float angularDamping() const { return angularDamping_; }
    const glm::vec3& velocity() const { return velocity_; }
    const glm::vec3& angularVelocity() const { return angularVelocity_; }
    bool planar() const { return planar_; }
    bool asleep() const { return asleep_; }
    float sleepTimer() const { return sleepTimer_; }

    RigidBodyComponent& setMotion(Motion motion) { motion_ = motion; return *this; }
    RigidBodyComponent& setMass(float mass) { mass_ = std::max(mass, MIN_MASS); return *this; }
    RigidBodyComponent& setGravityScale(float scale) { gravityScale_ = scale; return *this; }
    RigidBodyComponent& setDamping(float linear, float angular) {
        linearDamping_ = std::max(linear, 0.0f);
        angularDamping_ = std::max(angular, 0.0f);
        return *this;
    }
    RigidBodyComponent& setVelocity(float x, float y, float z) { velocity_ = glm::vec3(x, y, z); return *this; }
    RigidBodyComponent& setAngularVelocity(float x, float y, float z) {
        angularVelocity_ = glm::vec3(x, y, z);
        return *this;
    }
    RigidBodyComponent& setPlanar(bool planar) { planar_ = planar; return *this; }

    RigidBodyComponent& wake();

    RigidBodyComponent& applyImpulse(const glm::vec3& impulse);
    RigidBodyComponent& applyImpulse(const glm::vec3& impulse, const glm::vec3& point);
    RigidBodyComponent& applyForce(const glm::vec3& force);
    RigidBodyComponent& applyForce(const glm::vec3& force, const glm::vec3& point);
    RigidBodyComponent& applyTorque(const glm::vec3& torque);

    glm::vec3 center();
    cinder::Scene::Transform* transform() const;
    cinder::Scene::Transform* parentTransform() const;

    CINDER_COMPONENT(RigidBodyComponent, cinder::Scene::Component) {
        CINDER_PROP(motion_);
        CINDER_PROP_R(mass_, MIN_MASS, FLT_MAX);
        CINDER_PROP(gravityScale_);
        CINDER_PROP_R(linearDamping_, 0.0f, FLT_MAX);
        CINDER_PROP_R(angularDamping_, 0.0f, FLT_MAX);
        CINDER_PROP(velocity_);
        CINDER_PROP(angularVelocity_);
        CINDER_PROP(planar_);
    }

private:
    friend class World;

    Motion motion_ = Motion::Dynamic;
    float mass_ = 1.0f;
    float gravityScale_ = 1.0f;
    float linearDamping_ = 0.0f;
    float angularDamping_ = 0.0f;
    glm::vec3 velocity_{0.0f};
    glm::vec3 angularVelocity_{0.0f};
    bool planar_ = false;

    glm::vec3 force_{0.0f};
    glm::vec3 torque_{0.0f};
    glm::vec3 impulse_{0.0f};
    glm::vec3 angularImpulse_{0.0f};

    bool asleep_ = false;
    float sleepTimer_ = 0.0f;
    glm::vec3 wroteVelocity_{0.0f};
    glm::vec3 wroteSpin_{0.0f};

    glm::vec3 origin_{0.0f};
    glm::quat orientation_{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 center_{0.0f};
    glm::mat4 placed_{0.0f};
    bool simulated_ = false;
};

}

CINDER_ENUM_NAMES(cinder::Physics::RigidBodyComponent::Motion, "static", "kinematic", "dynamic")
