#include "game/Game.hpp"

#include <algorithm>

using namespace cinder;

class Flycam final : public scene::Node {
public:
    CINDER_NODE(Flycam, scene::Node) {
        CINDER_PROP(sensitivity_);
        CINDER_PROP(speed_);
        CINDER_PROP(sprint_);
    }

    void onStart() override {
        if (scene::Transform* camera = target()) {
            pitch_ = camera->rotation().x;
            yaw_ = camera->rotation().y;
        }
        platform::logInfo("[game] click to capture the mouse, wasd + space/ctrl to fly, esc to release\n");
    }

    void onUpdate(float dt) override {
        scene::Transform* camera = target();
        if (camera == nullptr) return;

        if (game::mousePressed("left") && !game::cursorLocked()) game::setCursorLocked(true);
        if (game::keyPressed("escape")) {
            if (game::cursorLocked()) game::setCursorLocked(false);
            else game::quit();
        }

        if (game::cursorLocked()) {
            const glm::vec2 delta = game::mouseDelta();
            yaw_ -= delta.x * sensitivity_;
            pitch_ = std::clamp(pitch_ - delta.y * sensitivity_, -1.55f, 1.55f);
            camera->setRotation(pitch_, yaw_, 0.0f);
        }

        glm::vec3 move(0.0f);
        if (game::keyDown("w")) move += camera->forward();
        if (game::keyDown("s")) move -= camera->forward();
        if (game::keyDown("d")) move += camera->right();
        if (game::keyDown("a")) move -= camera->right();
        if (game::keyDown("space")) move.y += 1.0f;
        if (game::keyDown("lctrl")) move.y -= 1.0f;

        const glm::vec3 step = move * ((game::keyDown("lshift") ? sprint_ : speed_) * dt);
        camera->translate(step.x, step.y, step.z);
    }

private:
    scene::Transform* target() { return parent() != nullptr ? parent()->transform() : nullptr; }

    float sensitivity_ = 0.0021f;
    float speed_ = 8.0f;
    float sprint_ = 24.0f;
    float pitch_ = 0.0f;
    float yaw_ = 0.0f;
};

CINDER_REGISTER(Flycam);
