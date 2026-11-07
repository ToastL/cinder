#include "Game/Game.hpp"

using namespace cinder;

class Dropper final : public Scene::Node {
public:
    CINDER_NODE(Dropper, Scene::Node) {
        CINDER_PROP(interval_);
        CINDER_PROP(lifetime_);
        CINDER_PROP(spread_);
    }

    void onStart() override {
        Game::scene().every(
            *this,
            interval_,
            [this] {
                drop();
            }
        );
    }

private:
    void drop() {
        Scene::Transform* from = parent() != nullptr ? parent()->transform() : nullptr;
        if (from == nullptr) return;

        Scene::Scene& world = Game::scene();
        const glm::vec3 at = from->worldPosition()
                + glm::vec3(Game::random(-0.5f, 0.5f) * spread_, 0.0f, Game::random(-0.5f, 0.5f) * spread_);

        auto* ball = world.create<Physics::Body>(nullptr);
        ball->setName("Dropped");
        ball->transform()->setPosition(at.x, at.y, at.z);
        ball->transform()->setScale(0.8f);
        ball->setDamping(0.2f, ball->angularDamping());
        world.create<Nodes::MeshPart>(ball)->setMesh("sphere");
        world.create<Physics::Collider>(ball)->setShape(Physics::Shape::Sphere).setRestitution(0.5f);

        const int id = ball->id();
        world.after(
            *ball,
            lifetime_,
            [&world, id] {
                world.destroy(world.byId(id));
            }
        );
    }

    float interval_ = 0.7f;
    float lifetime_ = 9.0f;
    float spread_ = 4.0f;
};

CINDER_REGISTER(Dropper);
