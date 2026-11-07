#include "game/Game.hpp"

using namespace cinder;

class ClickSpawner final : public scene::Node {
public:
    CINDER_NODE(ClickSpawner, scene::Node) { CINDER_PROP(lifetime_); }

    void onStart() override { platform::logInfo("[game] click to drop a box\n"); }

    void onUpdate(float dt) override {
        if (!game::mousePressed("left")) return;

        scene::Scene& world = game::scene();
        const glm::vec2 at = game::screenToWorld(game::mousePosition());

        auto* box = world.create<physics::Body>(nullptr);
        box->setName("Dropped");
        box->setPlanar(true);
        box->transform()->setPosition(at.x, at.y, 0.0f);
        world.create<nodes::Sprite>(box)
                ->setSize(1.0f, 1.0f)
                .setColor(game::random(0.4f, 1.0f), game::random(0.4f, 1.0f), 0.9f, 1.0f);
        world.create<physics::Collider>(box);

        const int id = box->id();
        world.after(*box, lifetime_, [&world, id] { world.destroy(world.byId(id)); });
    }

private:
    float lifetime_ = 12.0f;
};

CINDER_REGISTER(ClickSpawner);
