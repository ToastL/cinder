#include "Game/Game.hpp"

using namespace cinder;

class Spinner final : public Scene::Node {
public:
    CINDER_NODE(Spinner, Scene::Node) { CINDER_PROP(speed_); }

    void onUpdate(float dt) override {
        Scene::Transform* box = parent() != nullptr ? parent()->transform() : nullptr;
        if (box == nullptr) return;
        const glm::vec3 rotation = box->rotation() + speed_ * dt;
        box->setRotation(rotation.x, rotation.y, rotation.z);
    }

private:
    glm::vec3 speed_{0.0f, 1.0f, 0.0f};
};

CINDER_REGISTER(Spinner);
