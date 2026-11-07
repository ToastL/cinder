#include "Game/Game.hpp"

#include <cmath>

using namespace cinder;

class Bobber final : public Scene::Component {
public:
    CINDER_COMPONENT(Bobber, Scene::Component) {
        CINDER_PROP(phase_);
        CINDER_PROP(rate_);
    }

    void tick(float dt) override {
        Scene::Transform* box = owner()->transform();
        if (box == nullptr) return;
        const float height = 1.0f + std::sin(static_cast<float>(Game::time()) * rate_ + phase_);
        box->setPosition(box->position().x, height * 0.5f, box->position().z);
        box->setScale(1.0f, height, 1.0f);
    }

private:
    float phase_ = 0.0f;
    float rate_ = 2.0f;
};

CINDER_REGISTER(Bobber);
