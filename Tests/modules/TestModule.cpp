#include "Game/Game.hpp"

class TestTicker final : public cinder::Scene::Component {
public:
    CINDER_COMPONENT(TestTicker, cinder::Scene::Component) { CINDER_PROP(step_); }

    void beginPlay() override { scene()->spawn<cinder::Scene::Actor>(owner())->setName("Made"); }

    void tick(float dt) override {
        count_ += step_;
        owner()->setAttribute("count", cinder::Scene::PropValue::number(count_));
    }

private:
    float step_ = 1.0f;
    double count_ = 0.0;
};

class TestMover final : public cinder::Scene::SceneComponent {
public:
    CINDER_COMPONENT(TestMover, cinder::Scene::SceneComponent) { CINDER_PROP(speed_); }

    void tick(float dt) override {
        const glm::vec3 position = transform().position();
        transform().setPosition(position.x + speed_ * dt, position.y, position.z);
    }

private:
    float speed_ = 2.0f;
};

class Actor final : public cinder::Scene::Component {
public:
    CINDER_COMPONENT(Actor, cinder::Scene::Component) {}
};

CINDER_REGISTER(TestTicker);
CINDER_REGISTER(TestMover);
CINDER_REGISTER(Actor);
CINDER_MODULE_ENTRY()
