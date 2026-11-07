#include "Game/Game.hpp"

using namespace cinder;

class Dropper final : public Scene::Component {
public:
    CINDER_COMPONENT(Dropper, Scene::Component) {
        CINDER_PROP(interval_);
        CINDER_PROP(lifetime_);
        CINDER_PROP(spread_);
    }

    void beginPlay() override {
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
        Scene::Transform* from = owner()->transform();
        if (from == nullptr) return;

        Scene::Scene& world = Game::scene();
        const glm::vec3 at = from->worldPosition()
                + glm::vec3(Game::random(-0.5f, 0.5f) * spread_, 0.0f, Game::random(-0.5f, 0.5f) * spread_);

        auto* ball = world.spawn<Components::StaticMeshActor>();
        ball->setName("Dropped");
        ball->mesh().setMesh("sphere");
        ball->transform()->setPosition(at.x, at.y, at.z);
        ball->transform()->setScale(0.8f);
        auto* body = ball->addComponent<Physics::RigidBodyComponent>();
        body->setDamping(0.2f, body->angularDamping());
        ball->addComponent<Physics::ColliderComponent>()->setShape(Physics::Shape::Sphere).setRestitution(0.5f);

        const int id = ball->id();
        world.after(
            *ball,
            lifetime_,
            [&world, id] {
                world.destroy(world.actor(id));
            }
        );
    }

    float interval_ = 0.7f;
    float lifetime_ = 9.0f;
    float spread_ = 4.0f;
};

CINDER_REGISTER(Dropper);
