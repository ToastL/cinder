#pragma once

#include "Gfx/RHI/Fwd.hpp"
#include "Scene/SceneComponent.hpp"

#include <memory>

class CloudComponent final : public cinder::Scene::SceneComponent {
public:
    CINDER_COMPONENT(CloudComponent, cinder::Scene::SceneComponent) {
        CINDER_PROP_S(sunYaw_, -180.0f, 180.0f, 1.0f);
        CINDER_PROP_S(sunPitch_, -90.0f, 90.0f, 1.0f);
        CINDER_PROP_R(coverage_, 0.0f, 1.0f);
        CINDER_PROP_R(density_, 0.0f, 5.0f);
        CINDER_PROP_R(windSpeed_, -10.0f, 10.0f);
    }

    CloudComponent();
    ~CloudComponent() override;

    CloudComponent(const CloudComponent&) = delete;
    CloudComponent& operator=(const CloudComponent&) = delete;

private:
    float sunYaw_ = -45.0f;
    float sunPitch_ = 25.0f;
    float coverage_ = 0.75f;
    float density_ = 4.0f;
    float windSpeed_ = 0.4f;

    std::unique_ptr<cinder::Gfx::RHI::GraphicsPipeline> pipeline_;
};
