#pragma once

#include "gfx/pass/PerspectiveCamera.hpp"

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace cinder::components { class Camera; }

namespace cinder::dev {

class EditorCamera {
public:
    struct Controls {
        glm::vec2 look{0.0f};
        glm::vec2 pan{0.0f};
        glm::vec3 move{0.0f};
        float scroll = 0.0f;
        bool looking = false;
        bool fast = false;
    };

    bool seeded() const { return seeded_; }
    void seed(cinder::components::Camera* camera);

    void setAspect(float aspect) { camera_.setAspect(aspect); }
    void update(const Controls& controls, float dt);

    cinder::gfx::pass::PerspectiveCamera& camera() { return camera_; }

private:
    glm::mat4 orientation() const;
    void place();

    cinder::gfx::pass::PerspectiveCamera camera_;
    glm::vec3 position_{0.0f};
    float yaw_ = 0.0f;
    float pitch_ = 0.0f;
    float speed_ = 5.0f;
    bool seeded_ = false;
};

}
