#include "game/Game.hpp"

class TestTicker final : public cinder::scene::Node {
public:
    CINDER_NODE(TestTicker, cinder::scene::Node) { CINDER_PROP(step_); }

    void onStart() override { scene()->create<cinder::components::Group>(this)->setName("Made"); }

    void onUpdate(float dt) override {
        count_ += step_;
        setAttribute("count", cinder::scene::PropValue::number(count_));
    }

private:
    float step_ = 1.0f;
    double count_ = 0.0;
};

class TestMover final : public cinder::scene::Spatial {
public:
    CINDER_NODE(TestMover, cinder::scene::Spatial) { CINDER_PROP(speed_); }

    void onUpdate(float dt) override {
        const glm::vec3 position = transform()->position();
        transform()->setPosition(position.x + speed_ * dt, position.y, position.z);
    }

private:
    float speed_ = 2.0f;
};

class Group final : public cinder::scene::Node {
public:
    CINDER_NODE(Group, cinder::scene::Node) {}
};

CINDER_REGISTER(TestTicker);
CINDER_REGISTER(TestMover);
CINDER_REGISTER(Group);
CINDER_MODULE_ENTRY()
