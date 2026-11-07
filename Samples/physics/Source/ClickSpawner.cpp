#include "Game/Game.hpp"

using namespace cinder;

class ClickSpawner final : public Scene::Node {
public:
    CINDER_NODE(ClickSpawner, Scene::Node) { CINDER_PROP(lifetime_); }

    void onStart() override { Platform::logInfo("[game] click to drop a box\n"); }

    void onUpdate(float dt) override {
        if (!Game::mousePressed("left")) return;

        Scene::Scene& world = Game::scene();
        const glm::vec2 at = Game::screenToWorld(Game::mousePosition());

        auto* box = world.create<Physics::Body>(nullptr);
        box->setName("Dropped");
        box->setPlanar(true);
        box->transform()->setPosition(at.x, at.y, 0.0f);
        world.create<Nodes::Sprite>(box)
                ->setSize(1.0f, 1.0f)
                .setColor(Game::random(0.4f, 1.0f), Game::random(0.4f, 1.0f), 0.9f, 1.0f);
        world.create<Physics::Collider>(box);

        const int id = box->id();
        world.after(*box, lifetime_, [&world, id] { world.destroy(world.byId(id)); });
    }

private:
    float lifetime_ = 12.0f;
};

CINDER_REGISTER(ClickSpawner);
