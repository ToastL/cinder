#include "Game/Game.hpp"

using namespace cinder;

class FollowMouse final : public Scene::Component {
public:
    CINDER_COMPONENT(FollowMouse, Scene::Component) {}

    void tick(float dt) override {
        Scene::Transform* cursor = owner()->transform();
        if (cursor == nullptr) return;
        const glm::vec2 world = Game::screenToWorld(Game::mousePosition());
        cursor->setPosition(world.x, world.y, 0.0f);
    }
};

CINDER_REGISTER(FollowMouse);
