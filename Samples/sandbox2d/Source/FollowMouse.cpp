#include "game/Game.hpp"

using namespace cinder;

class FollowMouse final : public scene::Node {
public:
    CINDER_NODE(FollowMouse, scene::Node) {}

    void onUpdate(float dt) override {
        scene::Transform* cursor = parent() != nullptr ? parent()->transform() : nullptr;
        if (cursor == nullptr) return;
        const glm::vec2 world = game::screenToWorld(game::mousePosition());
        cursor->setPosition(world.x, world.y, 0.0f);
    }
};

CINDER_REGISTER(FollowMouse);
