#pragma once

#include "game/Game.hpp"

class Riser final : public cinder::scene::Node {
public:
    CINDER_NODE(Riser, cinder::scene::Node) { CINDER_PROP(speed_); }

    Riser& setSpeed(float speed) {
        speed_ = speed;
        return *this;
    }

    void onUpdate(float dt) override {
        if (cinder::scene::Transform* box = parent() != nullptr ? parent()->transform() : nullptr) {
            box->translate(0.0f, speed_ * dt, 0.0f);
        }
    }

private:
    float speed_ = 50.0f;
};
