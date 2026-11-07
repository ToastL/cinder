#include "Game/Game.hpp"

#include <algorithm>

using namespace cinder;

class Flycam final : public Scene::Component {
public:
    CINDER_COMPONENT(Flycam, Scene::Component) {
        CINDER_PROP(sensitivity_);
        CINDER_PROP(speed_);
        CINDER_PROP(sprint_);
    }

    void beginPlay() override {
        if (Scene::Transform* camera = target()) {
            pitch_ = camera->rotation().x;
            yaw_ = camera->rotation().y;
        }
        Platform::logInfo("[game] click to capture the mouse, wasd + space/ctrl to fly, esc to release\n");
    }

    void tick(float dt) override {
        Scene::Transform* camera = target();
        if (camera == nullptr) return;

        if (Game::mousePressed("left") && !Game::cursorLocked()) Game::setCursorLocked(true);
        if (Game::keyPressed("escape")) {
            if (Game::cursorLocked()) Game::setCursorLocked(false);
            else Game::quit();
        }

        if (Game::cursorLocked()) {
            const glm::vec2 delta = Game::mouseDelta();
            yaw_ -= delta.x * sensitivity_;
            pitch_ = std::clamp(pitch_ - delta.y * sensitivity_, -1.55f, 1.55f);
            camera->setRotation(pitch_, yaw_, 0.0f);
        }

        glm::vec3 move(0.0f);
        if (Game::keyDown("w")) move += camera->forward();
        if (Game::keyDown("s")) move -= camera->forward();
        if (Game::keyDown("d")) move += camera->right();
        if (Game::keyDown("a")) move -= camera->right();
        if (Game::keyDown("space")) move.y += 1.0f;
        if (Game::keyDown("lctrl")) move.y -= 1.0f;

        const glm::vec3 step = move * ((Game::keyDown("lshift") ? sprint_ : speed_) * dt);
        camera->translate(step.x, step.y, step.z);
    }

private:
    Scene::Transform* target() { return owner()->transform(); }

    float sensitivity_ = 0.0021f;
    float speed_ = 8.0f;
    float sprint_ = 24.0f;
    float pitch_ = 0.0f;
    float yaw_ = 0.0f;
};

CINDER_REGISTER(Flycam);
