#pragma once

#include "reflect/Enum.hpp"
#include "scene/Spatial.hpp"

#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <cfloat>

namespace cinder::physics {

class Body final : public cinder::scene::Spatial {
public:
    enum class Motion { Static, Kinematic, Dynamic };

    Motion motion() const { return motion_; }
    float mass() const { return mass_; }
    float gravityScale() const { return gravityScale_; }
    float linearDamping() const { return linearDamping_; }
    float angularDamping() const { return angularDamping_; }
    const glm::vec3& velocity() const { return velocity_; }
    const glm::vec3& angularVelocity() const { return angularVelocity_; }
    bool planar() const { return planar_; }

    Body& setMotion(Motion motion) { motion_ = motion; return *this; }
    Body& setMass(float mass) { mass_ = mass; return *this; }
    Body& setGravityScale(float scale) { gravityScale_ = scale; return *this; }
    Body& setDamping(float linear, float angular) {
        linearDamping_ = linear;
        angularDamping_ = angular;
        return *this;
    }
    Body& setVelocity(float x, float y, float z) { velocity_ = glm::vec3(x, y, z); return *this; }
    Body& setAngularVelocity(float x, float y, float z) {
        angularVelocity_ = glm::vec3(x, y, z);
        return *this;
    }
    Body& setPlanar(bool planar) { planar_ = planar; return *this; }

    Body& applyImpulse(const glm::vec3& impulse);
    Body& applyImpulse(const glm::vec3& impulse, const glm::vec3& point);
    Body& applyForce(const glm::vec3& force);
    Body& applyForce(const glm::vec3& force, const glm::vec3& point);
    Body& applyTorque(const glm::vec3& torque);

    glm::vec3 centre();

    CINDER_NODE(Body, cinder::scene::Spatial) {
        CINDER_PROP(motion_);
        CINDER_PROP_R(mass_, 0.001f, FLT_MAX);
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

    glm::vec3 origin_{0.0f};
    glm::quat orientation_{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 center_{0.0f};
    glm::mat4 placed_{0.0f};
    bool simulated_ = false;
};

}

CINDER_ENUM_NAMES(cinder::physics::Body::Motion, "static", "kinematic", "dynamic")
