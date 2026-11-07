#include "Game/Game.hpp"

using namespace cinder;

class PanCamera final : public Scene::Component {
public:
    CINDER_COMPONENT(PanCamera, Scene::Component) { CINDER_PROP(speed_); }

    void beginPlay() override { Platform::logInfo("[game] wasd/arrows pan, scroll zooms, click spawns\n"); }

    void tick(float dt) override {
        auto* camera = owner()->findComponent<Components::CameraComponent>();
        if (camera == nullptr) return;
        if (Game::keyPressed("escape")) Game::quit();

        glm::vec3 move(0.0f);
        if (Game::keyDown("a") || Game::keyDown("left")) move.x -= 1.0f;
        if (Game::keyDown("d") || Game::keyDown("right")) move.x += 1.0f;
        if (Game::keyDown("w") || Game::keyDown("up")) move.y += 1.0f;
        if (Game::keyDown("s") || Game::keyDown("down")) move.y -= 1.0f;

        const glm::vec3 step = move * (speed_ * dt / camera->zoom());
        camera->transform().translate(step.x, step.y, step.z);

        const float scroll = Game::scroll().y;
        if (scroll != 0.0f) camera->setZoom(camera->zoom() * (1.0f + scroll * 0.1f));
    }

private:
    float speed_ = 200.0f;
};

CINDER_REGISTER(PanCamera);
