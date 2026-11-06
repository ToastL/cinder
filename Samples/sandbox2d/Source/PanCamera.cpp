#include "game/Game.hpp"

using namespace cinder;

class PanCamera final : public scene::Node {
public:
    CINDER_NODE(PanCamera, scene::Node) { CINDER_PROP(speed_); }

    void onStart() override { platform::logInfo("[game] wasd/arrows pan, scroll zooms, click spawns\n"); }

    void onUpdate(float dt) override {
        auto* camera = dynamic_cast<components::Camera*>(parent());
        if (camera == nullptr) return;
        if (game::keyPressed("escape")) game::quit();

        glm::vec3 move(0.0f);
        if (game::keyDown("a") || game::keyDown("left")) move.x -= 1.0f;
        if (game::keyDown("d") || game::keyDown("right")) move.x += 1.0f;
        if (game::keyDown("w") || game::keyDown("up")) move.y += 1.0f;
        if (game::keyDown("s") || game::keyDown("down")) move.y -= 1.0f;

        const glm::vec3 step = move * (speed_ * dt / camera->zoom());
        camera->transform()->translate(step.x, step.y, step.z);

        const float scroll = game::scroll().y;
        if (scroll != 0.0f) camera->setZoom(camera->zoom() * (1.0f + scroll * 0.1f));
    }

private:
    float speed_ = 200.0f;
};

CINDER_REGISTER(PanCamera);
