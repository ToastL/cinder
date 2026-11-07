#pragma once

#include "Game/Game.hpp"

class Riser final : public cinder::Scene::Component {
public:
    CINDER_COMPONENT(Riser, cinder::Scene::Component) { CINDER_PROP(speed_); }

    Riser& setSpeed(float speed) {
        speed_ = speed;
        return *this;
    }

    void tick(float dt) override {
        if (cinder::Scene::Transform* box = owner()->transform()) {
            box->translate(0.0f, speed_ * dt, 0.0f);
        }
    }

private:
    float speed_ = 50.0f;
};
