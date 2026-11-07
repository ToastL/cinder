#include "Game/Game.hpp"

using namespace cinder;

class FollowMouse final : public Scene::Node {
public:
    CINDER_NODE(FollowMouse, Scene::Node) {}

    void onUpdate(float dt) override {
        Scene::Transform* cursor = parent() != nullptr ? parent()->transform() : nullptr;
        if (cursor == nullptr) return;
        const glm::vec2 world = Game::screenToWorld(Game::mousePosition());
        cursor->setPosition(world.x, world.y, 0.0f);
    }
};

CINDER_REGISTER(FollowMouse);
