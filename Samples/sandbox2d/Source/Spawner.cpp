#include "Riser.hpp"
#include "Game/Game.hpp"

using namespace cinder;

class Spawner final : public Scene::Component {
public:
    CINDER_COMPONENT(Spawner, Scene::Component) {
        CINDER_PROP(lifetime_);
        CINDER_PROP(minSize_);
        CINDER_PROP(maxSize_);
    }

    void tick(float dt) override {
        if (!Game::mouseDown("left")) return;

        Scene::Scene& world = Game::scene();
        const glm::vec2 at = Game::screenToWorld(Game::mousePosition());
        const float size = Game::random(minSize_, maxSize_);

        auto* box = world.spawn<Components::SpriteActor>();
        box->setName("Box");
        box->transform()->setPosition(at.x, at.y, 0.0f);
        box->sprite().setSize(size, size).setColor(Game::random(), Game::random(), Game::random(), 0.85f);
        box->addComponent<Riser>()->setSpeed(Game::random(20.0f, 100.0f));

        const int id = box->id();
        world.after(*box, lifetime_, [&world, id] { world.destroy(world.actor(id)); });
    }

private:
    float lifetime_ = 6.0f;
    float minSize_ = 4.0f;
    float maxSize_ = 18.0f;
};

CINDER_REGISTER(Spawner);
