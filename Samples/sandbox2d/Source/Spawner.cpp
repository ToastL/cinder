#include "Riser.hpp"
#include "game/Game.hpp"

using namespace cinder;

class Spawner final : public scene::Node {
public:
    CINDER_NODE(Spawner, scene::Node) {
        CINDER_PROP(lifetime_);
        CINDER_PROP(minSize_);
        CINDER_PROP(maxSize_);
    }

    void onUpdate(float dt) override {
        if (!game::mouseDown("left")) return;

        scene::Scene& world = game::scene();
        const glm::vec2 at = game::screenToWorld(game::mousePosition());
        const float size = game::random(minSize_, maxSize_);

        auto* box = world.create<components::Sprite>(nullptr);
        box->setName("Box");
        box->transform()->setPosition(at.x, at.y, 0.0f);
        box->setSize(size, size);
        box->setColor(game::random(), game::random(), game::random(), 0.85f);
        world.create<Riser>(box)->setSpeed(game::random(20.0f, 100.0f));

        const int id = box->id();
        world.after(*box, lifetime_, [&world, id] { world.destroy(world.byId(id)); });
    }

private:
    float lifetime_ = 6.0f;
    float minSize_ = 4.0f;
    float maxSize_ = 18.0f;
};

CINDER_REGISTER(Spawner);
