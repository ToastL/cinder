#include "Game/Game.hpp"

using namespace cinder;

class ClickSpawner final : public Scene::Component {
public:
    CINDER_COMPONENT(ClickSpawner, Scene::Component) { CINDER_PROP(lifetime_); }

    void beginPlay() override { Platform::logInfo("[game] click to drop a box\n"); }

    void tick(float dt) override {
        if (!Game::mousePressed("left")) return;

        Scene::Scene& world = Game::scene();
        const glm::vec2 at = Game::screenToWorld(Game::mousePosition());

        auto* box = world.spawn<Components::SpriteActor>();
        box->setName("Dropped");
        box->transform()->setPosition(at.x, at.y, 0.0f);
        box->sprite()
                .setSize(1.0f, 1.0f)
                .setColor(Game::random(0.4f, 1.0f), Game::random(0.4f, 1.0f), 0.9f, 1.0f);
        box->addComponent<Physics::RigidBodyComponent>()->setPlanar(true);
        box->addComponent<Physics::ColliderComponent>();

        const int id = box->id();
        world.after(*box, lifetime_, [&world, id] { world.destroy(world.actor(id)); });
    }

private:
    float lifetime_ = 12.0f;
};

CINDER_REGISTER(ClickSpawner);
