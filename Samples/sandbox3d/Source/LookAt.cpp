#include "Game/Game.hpp"

#include <cmath>

using namespace cinder;

class LookAt final : public Scene::Component {
public:
    CINDER_COMPONENT(LookAt, Scene::Component) { CINDER_PROP(target_); }

    void tick(float) override {
        Scene::Actor* target = target_.get(*scene());
        Scene::Transform* self = owner()->transform();
        if (target == nullptr || target->transform() == nullptr || self == nullptr) return;
        const glm::vec3 to = target->transform()->worldPosition() - self->worldPosition();
        const float yaw = std::atan2(-to.x, -to.z);
        const float pitch = std::atan2(to.y, std::sqrt(to.x * to.x + to.z * to.z));
        self->setRotation(pitch, yaw, 0.0f);
    }

private:
    Scene::Ref<Scene::Actor> target_;
};

CINDER_REGISTER(LookAt);
