#include "Game/Game.hpp"

class TestTicker final : public cinder::Scene::Node {
public:
    CINDER_NODE(TestTicker, cinder::Scene::Node) { CINDER_PROP(step_); }

    void onStart() override { scene()->create<cinder::Nodes::Group>(this)->setName("Made"); }

    void onUpdate(float dt) override {
        count_ += step_;
        setAttribute("count", cinder::Scene::PropValue::number(count_));
    }

private:
    float step_ = 1.0f;
    double count_ = 0.0;
};

class TestMover final : public cinder::Scene::Spatial {
public:
    CINDER_NODE(TestMover, cinder::Scene::Spatial) { CINDER_PROP(speed_); }

    void onUpdate(float dt) override {
        const glm::vec3 position = transform()->position();
        transform()->setPosition(position.x + speed_ * dt, position.y, position.z);
    }

private:
    float speed_ = 2.0f;
};

class Group final : public cinder::Scene::Node {
public:
    CINDER_NODE(Group, cinder::Scene::Node) {}
};

CINDER_REGISTER(TestTicker);
CINDER_REGISTER(TestMover);
CINDER_REGISTER(Group);
CINDER_MODULE_ENTRY()
