#pragma once

#include "Gfx/Pass/ViewCamera.hpp"
#include "Scene/View.hpp"

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace cinder::Nodes { class Camera; }

namespace cinder::Dev {

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
    void seed(cinder::Nodes::Camera* camera, glm::vec2 viewSize);

    void setViewSize(glm::vec2 size) { camera_.setViewSize(size.x, size.y); }
    void update(const Controls& controls, float dt);

    const cinder::Scene::View& view() const { return camera_.view(); }
    cinder::Gfx::Pass::ViewCamera& camera() { return camera_; }
    const cinder::Gfx::Pass::ViewCamera& camera() const { return camera_; }

private:
    glm::mat4 orientation() const;
    void place();

    cinder::Gfx::Pass::ViewCamera camera_;
    glm::vec3 position_{0.0f};
    float yaw_ = 0.0f;
    float pitch_ = 0.0f;
    float speed_ = 5.0f;
    bool seeded_ = false;
};

}
